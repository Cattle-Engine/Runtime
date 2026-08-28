#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <system_error>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace {

llvm::cl::OptionCategory ToolCategory("namespace-refactor options");

enum class OperationMode { Rename, Transfer };

llvm::cl::opt<OperationMode> Mode(
    "mode", llvm::cl::desc("Operation mode"),
    llvm::cl::values(
        clEnumValN(OperationMode::Rename, "rename", "Rename the namespace"),
        clEnumValN(OperationMode::Transfer, "transfer",
                   "Transfer a type from one namespace to another")),
    llvm::cl::init(OperationMode::Rename),
    llvm::cl::cat(ToolCategory));

llvm::cl::opt<std::string> OldNamespace(
    "old",
    llvm::cl::desc("Source/original namespace name"),
    llvm::cl::Required,
    llvm::cl::cat(ToolCategory));

llvm::cl::opt<std::string> NewNamespace(
    "new",
    llvm::cl::desc("Destination/replacement namespace name"),
    llvm::cl::Required,
    llvm::cl::cat(ToolCategory));

llvm::cl::opt<std::string> TypeName(
    "type",
    llvm::cl::desc("Type to transfer in --mode=transfer"),
    llvm::cl::cat(ToolCategory));

llvm::cl::opt<bool> DryRun(
    "dry-run",
    llvm::cl::desc("Print the replacements instead of writing files"),
    llvm::cl::init(false),
    llvm::cl::cat(ToolCategory));

std::string canonicalizePath(StringRef Path) {
    llvm::SmallString<256> Result(Path);

    if (llvm::sys::fs::real_path(Path, Result, /*expand_tilde=*/true))
        return std::string(Path);

    return std::string(Result.str());
}

void addReplacement(
    const SourceManager& SM,
    const LangOptions& LangOpts,
    CharSourceRange Range,
    StringRef NewText,
    std::map<std::string, Replacements>& ReplsByFile) {
    if (Range.isInvalid())
        return;

    if (Range.getBegin().isMacroID() || Range.getEnd().isMacroID())
        return;

    Replacement Repl(SM, Range, NewText, LangOpts);

    if (Repl.getFilePath().empty())
        return;

    std::string CanonicalPath = canonicalizePath(Repl.getFilePath());

    if (CanonicalPath != Repl.getFilePath()) {
        Repl = Replacement(
            CanonicalPath,
            Repl.getOffset(),
            Repl.getLength(),
            Repl.getReplacementText());
    }

    if (llvm::Error Err = ReplsByFile[CanonicalPath].add(Repl))
        llvm::consumeError(std::move(Err));
}

std::string getQualifiedNamespaceName(const NamespaceDecl* NS) {
    if (!NS)
        return {};

    return NS->getQualifiedNameAsString();
}

const NamespaceDecl* getNamespaceFromSpecifier(NestedNameSpecifier NNS) {
    if (!NNS ||
        NNS.getKind() != NestedNameSpecifier::Kind::Namespace) {
        return nullptr;
    }

    NamespaceAndPrefix NP = NNS.getAsNamespaceAndPrefix();

    return dyn_cast<NamespaceDecl>(NP.Namespace);
}

CharSourceRange getNamespaceQualifierRange(NestedNameSpecifierLoc QualLoc) {
    if (!QualLoc)
        return CharSourceRange();

    SourceRange Range = QualLoc.getSourceRange();

    if (Range.isInvalid())
        return CharSourceRange();

    return CharSourceRange::getTokenRange(Range);
}

/*
 * Returns the char range covering a whole type definition:
 *
 *     struct Foo {
 *     };
 *
 *     class Foo {
 *     };
 *
 *     union Foo {
 *     };
 *
 *     enum Foo {
 *     };
 */
CharSourceRange getTypeDefinitionRange(
    const TagDecl* TD,
    const SourceManager& SM,
    const LangOptions& LangOpts) {
    if (!TD || !TD->isThisDeclarationADefinition())
        return CharSourceRange();

    if (TD->getBraceRange().isInvalid())
        return CharSourceRange();

    SourceLocation Begin = TD->getBeginLoc();
    SourceLocation End = TD->getBraceRange().getEnd();

    SourceLocation StopLoc =
        Lexer::getLocForEndOfToken(End, 0, SM, LangOpts);

    if (auto NextTok = Lexer::findNextToken(End, SM, LangOpts)) {
        if (NextTok->is(tok::semi)) {
            StopLoc =
                Lexer::getLocForEndOfToken(
                    NextTok->getLocation(),
                    0,
                    SM,
                    LangOpts);
        }
    }

    return CharSourceRange::getCharRange(Begin, StopLoc);
}

std::string getSourceText(
    const SourceManager& SM,
    const LangOptions& LangOpts,
    CharSourceRange Range) {
    if (Range.isInvalid())
        return {};

    StringRef Text = Lexer::getSourceText(Range, SM, LangOpts);

    if (Text.empty())
        return {};

    return Text.str();
}

//===--------------------------- Rename mode ---------------------------===//

class NamespaceDeclHandler : public MatchFinder::MatchCallback {
  public:
    explicit NamespaceDeclHandler(
        std::map<std::string, Replacements>& Repls)
        : ReplsByFile(Repls) {
    }

    void run(const MatchFinder::MatchResult& Result) override {
        const auto* NS =
            Result.Nodes.getNodeAs<NamespaceDecl>("nsDecl");

        if (!NS || NS->isAnonymousNamespace())
            return;

        if (getQualifiedNamespaceName(NS) != OldNamespace)
            return;

        SourceLocation Loc = NS->getLocation();

        addReplacement(
            *Result.SourceManager,
            Result.Context->getLangOpts(),
            CharSourceRange::getTokenRange(Loc, Loc),
            NewNamespace,
            ReplsByFile);
    }

  private:
    std::map<std::string, Replacements>& ReplsByFile;
};

class UsingDirectiveHandler : public MatchFinder::MatchCallback {
  public:
    explicit UsingDirectiveHandler(
        std::map<std::string, Replacements>& Repls)
        : ReplsByFile(Repls) {
    }

    void run(const MatchFinder::MatchResult& Result) override {
        const auto* UD =
            Result.Nodes.getNodeAs<UsingDirectiveDecl>(
                "usingDirective");

        if (!UD || !UD->getNominatedNamespace())
            return;

        if (getQualifiedNamespaceName(
                UD->getNominatedNamespace()) != OldNamespace) {
            return;
        }

        SourceLocation Loc = UD->getIdentLocation();

        addReplacement(
            *Result.SourceManager,
            Result.Context->getLangOpts(),
            CharSourceRange::getTokenRange(Loc, Loc),
            NewNamespace,
            ReplsByFile);
    }

  private:
    std::map<std::string, Replacements>& ReplsByFile;
};

class NamespaceAliasHandler : public MatchFinder::MatchCallback {
  public:
    explicit NamespaceAliasHandler(
        std::map<std::string, Replacements>& Repls)
        : ReplsByFile(Repls) {
    }

    void run(const MatchFinder::MatchResult& Result) override {
        const auto* Alias =
            Result.Nodes.getNodeAs<NamespaceAliasDecl>("nsAlias");

        if (!Alias || !Alias->getNamespace())
            return;

        if (getQualifiedNamespaceName(Alias->getNamespace()) != OldNamespace)
            return;

        SourceLocation Loc = Alias->getTargetNameLoc();

        addReplacement(
            *Result.SourceManager,
            Result.Context->getLangOpts(),
            CharSourceRange::getTokenRange(Loc, Loc),
            NewNamespace,
            ReplsByFile);
    }

  private:
    std::map<std::string, Replacements>& ReplsByFile;
};

class QualifierHandler : public MatchFinder::MatchCallback {
  public:
    explicit QualifierHandler(
        std::map<std::string, Replacements>& Repls)
        : ReplsByFile(Repls) {
    }

    void run(const MatchFinder::MatchResult& Result) override {
        const auto* QualLoc =
            Result.Nodes.getNodeAs<NestedNameSpecifierLoc>(
                "qualifier");

        if (!QualLoc)
            return;

        NestedNameSpecifier NNS =
            QualLoc->getNestedNameSpecifier();

        const NamespaceDecl* NS =
            getNamespaceFromSpecifier(NNS);

        if (!NS)
            return;

        if (getQualifiedNamespaceName(NS) != OldNamespace)
            return;

        CharSourceRange Range =
            getNamespaceQualifierRange(*QualLoc);

        if (Range.isInvalid())
            return;

        addReplacement(
            *Result.SourceManager,
            Result.Context->getLangOpts(),
            Range,
            NewNamespace + "::",
            ReplsByFile);
    }

  private:
    std::map<std::string, Replacements>& ReplsByFile;
};

//===------------------------- Transfer mode ----------------------------===//

class TransferHandler : public MatchFinder::MatchCallback {
  public:
    explicit TransferHandler(
        std::map<std::string, Replacements>& Repls)
        : ReplsByFile(Repls) {
    }

    void onStartOfTranslationUnit() override {
        Declaration = nullptr;
        SourceNamespace = nullptr;
        DestinationCandidates.clear();
        Finalized = false;
    }

    void run(const MatchFinder::MatchResult& Result) override {
        if (const auto* TD =
                Result.Nodes.getNodeAs<TagDecl>("transferType")) {
            if (TD->getName() != TypeName ||
                !TD->isThisDeclarationADefinition()) {
                return;
            }

            const auto* NS =
                dyn_cast<NamespaceDecl>(TD->getDeclContext());

            if (!NS)
                return;

            if (getQualifiedNamespaceName(NS) != OldNamespace)
                return;

            if (Declaration && Declaration != TD)
                return;

            Declaration = TD;
            SourceNamespace = NS;
            return;
        }

        if (const auto* NS =
                Result.Nodes.getNodeAs<NamespaceDecl>(
                    "destinationNamespace")) {
            if (NS->isAnonymousNamespace())
                return;

            if (getQualifiedNamespaceName(NS) != NewNamespace)
                return;

            DestinationCandidates.push_back(NS);
            return;
        }

        if (const auto* TL =
                Result.Nodes.getNodeAs<TypeLoc>("typeLoc")) {
            if (auto TTL = TL->getAs<TagTypeLoc>()) {
                handleReference(
                    TTL,
                    *Result.SourceManager,
                    Result.Context->getLangOpts());
            }

            return;
        }
    }

    void onEndOfTranslationUnit() override {
        if (Finalized || !Declaration || !SourceNamespace)
            return;

        Finalized = true;

        const ASTContext& Ctx = Declaration->getASTContext();
        const SourceManager& SM = Ctx.getSourceManager();
        const LangOptions& LangOpts = Ctx.getLangOpts();

        std::string DeclFile =
            canonicalizePath(
                SM.getFilename(Declaration->getLocation()));

        if (!HandledDeclFiles.insert(DeclFile).second)
            return;

        CharSourceRange DeclRange =
            getTypeDefinitionRange(
                Declaration,
                SM,
                LangOpts);

        if (DeclRange.isInvalid() ||
            DeclRange.getBegin().isMacroID()) {
            return;
        }

        std::string TypeText =
            getSourceText(
                SM,
                LangOpts,
                DeclRange);

        if (TypeText.empty())
            return;

        addReplacement(
            SM,
            LangOpts,
            DeclRange,
            "",
            ReplsByFile);

        const NamespaceDecl* Destination = nullptr;

        for (const NamespaceDecl* Cand :
             DestinationCandidates) {
            if (canonicalizePath(
                    SM.getFilename(
                        Cand->getLocation())) == DeclFile) {
                Destination = Cand;
                break;
            }
        }

        if (Destination) {
            SourceLocation InsertLoc =
                Destination->getRBraceLoc();

            if (InsertLoc.isInvalid())
                return;

            std::string MovedText =
                "\n    " + TypeText + "\n";

            addReplacement(
                SM,
                LangOpts,
                CharSourceRange::getCharRange(
                    InsertLoc,
                    InsertLoc),
                MovedText,
                ReplsByFile);

            return;
        }

        SourceLocation AfterSrc =
            SourceNamespace->getRBraceLoc();

        if (AfterSrc.isInvalid())
            return;

        SourceLocation InsertLoc =
            Lexer::getLocForEndOfToken(
                AfterSrc,
                0,
                SM,
                LangOpts);

        std::string MovedText =
            "\n\nnamespace " +
            NewNamespace.getValue() +
            " {\n    " +
            TypeText +
            "\n} // namespace " +
            NewNamespace.getValue() +
            "\n";

        addReplacement(
            SM,
            LangOpts,
            CharSourceRange::getCharRange(
                InsertLoc,
                InsertLoc),
            MovedText,
            ReplsByFile);
    }

  private:
    void handleReference(
        const TagTypeLoc& TTL,
        const SourceManager& SM,
        const LangOptions& LangOpts) {
        NestedNameSpecifierLoc QualLoc =
            TTL.getQualifierLoc();

        if (!QualLoc)
            return;

        NestedNameSpecifier NNS =
            QualLoc.getNestedNameSpecifier();

        const NamespaceDecl* NS =
            getNamespaceFromSpecifier(NNS);

        if (!NS)
            return;

        /*
         * The namespace declaration carries its complete semantic
         * qualification, so this correctly recognizes:
         *
         *     CE::Assets::Skyboxes
         *
         * rather than merely:
         *
         *     Skyboxes
         */
        if (getQualifiedNamespaceName(NS) != OldNamespace)
            return;

        const TagDecl* Referenced =
            TTL.getDecl();

        if (!Referenced ||
            Referenced->getName() != TypeName) {
            return;
        }

        CharSourceRange Range =
            getNamespaceQualifierRange(QualLoc);

        if (Range.isInvalid())
            return;

        addReplacement(
            SM,
            LangOpts,
            Range,
            NewNamespace + "::",
            ReplsByFile);
    }

    std::map<std::string, Replacements>& ReplsByFile;

    const TagDecl* Declaration = nullptr;
    const NamespaceDecl* SourceNamespace = nullptr;

    std::vector<const NamespaceDecl*> DestinationCandidates;

    bool Finalized = false;

    /*
     * Persists across all translation units in the entire run.
     * Prevents a header from being transferred repeatedly when it
     * appears in multiple translation units.
     */
    std::set<std::string> HandledDeclFiles;
};

} // namespace

int main(int argc, const char** argv) {
    auto ExpectedParser =
        CommonOptionsParser::create(
            argc,
            argv,
            ToolCategory);

    if (!ExpectedParser) {
        llvm::errs()
            << llvm::toString(
                   ExpectedParser.takeError())
            << "\n";
        return 1;
    }

    CommonOptionsParser& OptionsParser =
        ExpectedParser.get();

    if (Mode == OperationMode::Transfer &&
        TypeName.empty()) {
        llvm::errs()
            << "error: --type is required in --mode=transfer\n";
        return 1;
    }

    RefactoringTool Tool(
        OptionsParser.getCompilations(),
        OptionsParser.getSourcePathList());

    std::map<std::string, Replacements>& ReplsByFile =
        Tool.getReplacements();

    MatchFinder Finder;

    NamespaceDeclHandler NSHandler(ReplsByFile);
    UsingDirectiveHandler UsingHandler(ReplsByFile);
    NamespaceAliasHandler AliasHandler(ReplsByFile);
    QualifierHandler QualHandler(ReplsByFile);
    TransferHandler TransferCallback(ReplsByFile);

    if (Mode == OperationMode::Rename) {
        /*
         * Match all namespace declarations and let the callback compare
         * their fully-qualified names. This supports both:
         *
         *     namespace Old
         *
         * and:
         *
         *     namespace CE::Assets::Skyboxes
         */
        Finder.addMatcher(
            namespaceDecl().bind("nsDecl"),
            &NSHandler);

        Finder.addMatcher(
            usingDirectiveDecl().bind("usingDirective"),
            &UsingHandler);

        Finder.addMatcher(
            namespaceAliasDecl().bind("nsAlias"),
            &AliasHandler);

        /*
         * Match all nested-name-specifiers. The callback checks whether
         * the complete namespace qualifier resolves to OldNamespace.
         */
        Finder.addMatcher(
            nestedNameSpecifierLoc().bind("qualifier"),
            &QualHandler);
    } else {
        Finder.addMatcher(
            tagDecl(
                hasName(TypeName),
                isDefinition())
                .bind("transferType"),
            &TransferCallback);

        /*
         * Match every namespace declaration and let the callback compare
         * its complete qualified name against NewNamespace.
         */
        Finder.addMatcher(
            namespaceDecl().bind("destinationNamespace"),
            &TransferCallback);

        /*
         * Clang 22 provides the generic typeLoc() matcher rather than the
         * removed elaboratedTypeLoc/tagTypeLoc matcher APIs.
         */
        Finder.addMatcher(
            typeLoc().bind("typeLoc"),
            &TransferCallback);
    }

    std::unique_ptr<FrontendActionFactory> Factory =
        newFrontendActionFactory(&Finder);

    int RunResult = Tool.run(Factory.get());

    if (RunResult != 0) {
        llvm::errs()
            << "warning: some files failed to parse; "
               "replacements for those files were skipped\n";
    }

    if (ReplsByFile.empty()) {
        if (Mode == OperationMode::Rename) {
            llvm::outs()
                << "No occurrences of namespace '"
                << OldNamespace
                << "' found.\n";
        } else {
            llvm::outs()
                << "No transferable definition of type '"
                << TypeName
                << "' found in namespace '"
                << OldNamespace
                << "'.\n";
        }

        return RunResult;
    }

    if (DryRun) {
        for (auto& FileAndRepls : ReplsByFile) {
            llvm::outs()
                << "== "
                << FileAndRepls.first
                << " ("
                << FileAndRepls.second.size()
                << " change(s)) ==\n";

            for (const Replacement& R :
                 FileAndRepls.second) {
                llvm::outs()
                    << "  offset "
                    << R.getOffset()
                    << ", length "
                    << R.getLength()
                    << " -> \""
                    << R.getReplacementText()
                    << "\"\n";
            }
        }

        return 0;
    }

    bool AnyFailures = false;

    for (auto& FileAndRepls :
         ReplsByFile) {
        const std::string& Path =
            FileAndRepls.first;

        const Replacements& Repls =
            FileAndRepls.second;

        llvm::ErrorOr<
            std::unique_ptr<llvm::MemoryBuffer>>
            BufferOrErr =
                llvm::MemoryBuffer::getFile(Path);

        if (!BufferOrErr) {
            llvm::errs()
                << "error: could not read "
                << Path
                << ": "
                << BufferOrErr.getError().message()
                << "\n";

            AnyFailures = true;
            continue;
        }

        llvm::Expected<std::string> NewCode =
            applyAllReplacements(
                (*BufferOrErr)->getBuffer(),
                Repls);

        if (!NewCode) {
            llvm::errs()
                << "error applying replacements to "
                << Path
                << ": "
                << llvm::toString(
                       NewCode.takeError())
                << "\n";

            AnyFailures = true;
            continue;
        }

        std::error_code EC;

        llvm::raw_fd_ostream Out(
            Path,
            EC,
            llvm::sys::fs::OF_None);

        if (EC) {
            llvm::errs()
                << "error: could not write "
                << Path
                << ": "
                << EC.message()
                << "\n";

            AnyFailures = true;
            continue;
        }

        Out << *NewCode;

        llvm::outs()
            << "Updated "
            << Path
            << " ("
            << Repls.size()
            << " change(s))\n";
    }

    return AnyFailures ? 1 : RunResult;
}