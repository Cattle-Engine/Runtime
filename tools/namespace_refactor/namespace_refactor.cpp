#include <map>
#include <memory>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace {

    llvm::cl::OptionCategory ToolCategory("namespace-refactor options");

    llvm::cl::opt<std::string> OldNamespace("old", llvm::cl::desc("Unqualified name of the namespace to rename"),
                                            llvm::cl::Required, llvm::cl::cat(ToolCategory));

    llvm::cl::opt<std::string> NewNamespace("new", llvm::cl::desc("Replacement namespace name"), llvm::cl::Required,
                                            llvm::cl::cat(ToolCategory));

    llvm::cl::opt<bool> DryRun("dry-run", llvm::cl::desc("Print the replacements instead of writing files"),
                               llvm::cl::init(false), llvm::cl::cat(ToolCategory));

    /// Registers a single-token replacement, skipping locations that can't be
    /// safely rewritten (invalid, or inside a macro expansion).
    void addReplacement(const SourceManager& SM, const LangOptions& LangOpts, CharSourceRange Range, StringRef NewText,
                        std::map<std::string, Replacements>& ReplsByFile) {
        if (Range.isInvalid())
            return;
        if (Range.getBegin().isMacroID() || Range.getEnd().isMacroID())
            return;

        Replacement Repl(SM, Range, NewText, LangOpts);
        if (Repl.getFilePath().empty())
            return;

        if (llvm::Error Err = ReplsByFile[std::string(Repl.getFilePath())].add(Repl)) {
            // Most common cause: an overlapping replacement was already added
            // (e.g. two matchers firing on the same token). Safe to ignore.
            llvm::consumeError(std::move(Err));
        }
    }

    /// Renames the identifier in `namespace Old { ... }` (including each
    /// component of a C++17 nested-namespace-definition like
    /// `namespace Old::Sub { ... }`).
    class NamespaceDeclHandler : public MatchFinder::MatchCallback {
      public:
        explicit NamespaceDeclHandler(std::map<std::string, Replacements>& Repls) : ReplsByFile(Repls) {}

        void run(const MatchFinder::MatchResult& Result) override {
            const auto* NS = Result.Nodes.getNodeAs<NamespaceDecl>("nsDecl");
            if (!NS || NS->isAnonymousNamespace())
                return;
            SourceLocation Loc = NS->getLocation();
            addReplacement(*Result.SourceManager, Result.Context->getLangOpts(),
                           CharSourceRange::getTokenRange(Loc, Loc), NewNamespace, ReplsByFile);
        }

      private:
        std::map<std::string, Replacements>& ReplsByFile;
    };

    /// Renames the namespace token in `using namespace Old;`.
    class UsingDirectiveHandler : public MatchFinder::MatchCallback {
      public:
        explicit UsingDirectiveHandler(std::map<std::string, Replacements>& Repls) : ReplsByFile(Repls) {}

        void run(const MatchFinder::MatchResult& Result) override {
            const auto* UD = Result.Nodes.getNodeAs<UsingDirectiveDecl>("usingDirective");
            if (!UD || !UD->getNominatedNamespace())
                return;
            if (UD->getNominatedNamespace()->getName() != OldNamespace)
                return;
            SourceLocation Loc = UD->getIdentLocation();
            addReplacement(*Result.SourceManager, Result.Context->getLangOpts(),
                           CharSourceRange::getTokenRange(Loc, Loc), NewNamespace, ReplsByFile);
        }

      private:
        std::map<std::string, Replacements>& ReplsByFile;
    };

    /// Renames the target of `namespace Alias = Old;`.
    class NamespaceAliasHandler : public MatchFinder::MatchCallback {
      public:
        explicit NamespaceAliasHandler(std::map<std::string, Replacements>& Repls) : ReplsByFile(Repls) {}

        void run(const MatchFinder::MatchResult& Result) override {
            const auto* Alias = Result.Nodes.getNodeAs<NamespaceAliasDecl>("nsAlias");
            if (!Alias || !Alias->getNamespace())
                return;
            if (Alias->getNamespace()->getName() != OldNamespace)
                return;
            // If the target was written with a qualifier (e.g. "= Foo::Old;")
            // that qualifier is handled by QualifierHandler instead; only rewrite
            // the trailing identifier here.
            SourceLocation Loc = Alias->getTargetNameLoc();
            addReplacement(*Result.SourceManager, Result.Context->getLangOpts(),
                           CharSourceRange::getTokenRange(Loc, Loc), NewNamespace, ReplsByFile);
        }

      private:
        std::map<std::string, Replacements>& ReplsByFile;
    };

    /// Renames a single qualifier component that refers to the target
    /// namespace, e.g. the "Old" in "Old::Foo", "Old::Sub::Thing",
    /// "using Old::Thing;", or "Old::Type value;".
    class QualifierHandler : public MatchFinder::MatchCallback {
      public:
        explicit QualifierHandler(std::map<std::string, Replacements>& Repls) : ReplsByFile(Repls) {}

        void run(const MatchFinder::MatchResult& Result) override {
            const auto* QualLoc = Result.Nodes.getNodeAs<NestedNameSpecifierLoc>("qualifier");
            if (!QualLoc)
                return;
            // getLocalSourceRange() covers just this component of the qualifier
            // (e.g. just "Old" in "Old::Sub::Thing"), not the trailing "::" or
            // any further components.
            CharSourceRange Range = CharSourceRange::getTokenRange(QualLoc->getLocalSourceRange());
            addReplacement(*Result.SourceManager, Result.Context->getLangOpts(), Range, NewNamespace, ReplsByFile);
        }

      private:
        std::map<std::string, Replacements>& ReplsByFile;
    };

} // namespace

int main(int argc, const char** argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << llvm::toString(ExpectedParser.takeError()) << "\n";
        return 1;
    }
    CommonOptionsParser& OptionsParser = ExpectedParser.get();

    RefactoringTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    std::map<std::string, Replacements>& ReplsByFile = Tool.getReplacements();

    MatchFinder Finder;
    NamespaceDeclHandler NSHandler(ReplsByFile);
    UsingDirectiveHandler UsingHandler(ReplsByFile);
    NamespaceAliasHandler AliasHandler(ReplsByFile);
    QualifierHandler QualHandler(ReplsByFile);

    Finder.addMatcher(namespaceDecl(hasName(OldNamespace)).bind("nsDecl"), &NSHandler);
    Finder.addMatcher(usingDirectiveDecl().bind("usingDirective"), &UsingHandler);
    Finder.addMatcher(namespaceAliasDecl().bind("nsAlias"), &AliasHandler);
    Finder.addMatcher(nestedNameSpecifierLoc(loc(specifiesNamespace(hasName(OldNamespace)))).bind("qualifier"),
                      &QualHandler);

    std::unique_ptr<FrontendActionFactory> Factory = newFrontendActionFactory(&Finder);

    int RunResult = Tool.run(Factory.get());
    if (RunResult != 0) {
        llvm::errs() << "warning: some files failed to parse; replacements for "
                        "those files were skipped\n";
    }

    if (ReplsByFile.empty()) {
        llvm::outs() << "No occurrences of namespace '" << OldNamespace << "' found.\n";
        return RunResult;
    }

    if (DryRun) {
        for (auto& FileAndRepls : ReplsByFile) {
            llvm::outs() << "== " << FileAndRepls.first << " (" << FileAndRepls.second.size() << " change(s)) ==\n";
            for (const Replacement& R : FileAndRepls.second) {
                llvm::outs() << "  offset " << R.getOffset() << ", length " << R.getLength() << " -> \""
                             << R.getReplacementText() << "\"\n";
            }
        }
        return 0;
    }

    // Re-run with the same factory via runAndSave() would re-parse; instead
    // apply the replacements we already collected directly.
    LangOptions DefaultLangOpts;
    SourceManager* LastSM = nullptr;
    (void)LastSM;

    // clang::tooling::applyAllReplacements needs a Rewriter bound to a
    // SourceManager per file; RefactoringTool provides a convenience helper
    // that does exactly that for every file with pending replacements.
    Rewriter Rewrite;
    // We need *a* SourceManager/FileManager pair to seed the Rewriter; build
    // a minimal one purely for on-disk file I/O.
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> DiagIDs(new DiagnosticIDs());
    DiagnosticOptions DiagOpts;

    DiagnosticsEngine Diagnostics(DiagIDs, DiagOpts, nullptr, false);
    FileSystemOptions FileMgrOpts;
    FileManager Files(FileMgrOpts);
    SourceManager SM(Diagnostics, Files);
    Rewrite.setSourceMgr(SM, DefaultLangOpts);

    bool AnyFailures = false;
    for (auto& FileAndRepls : ReplsByFile) {
        if (!applyAllReplacements(FileAndRepls.second, Rewrite)) {
            llvm::errs() << "error applying replacements to " << FileAndRepls.first << "\n";
            AnyFailures = true;
        }
    }

    if (Rewrite.overwriteChangedFiles()) {
        llvm::errs() << "error: failed to write one or more files\n";
        AnyFailures = true;
    } else {
        for (auto& FileAndRepls : ReplsByFile)
            llvm::outs() << "Updated " << FileAndRepls.first << " (" << FileAndRepls.second.size() << " change(s))\n";
    }

    return AnyFailures ? 1 : RunResult;
}
