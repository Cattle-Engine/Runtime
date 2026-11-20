#include "ce_editor/undoredo_system.hpp"

namespace CE::Editor {
    void UndoRedoSystem::TrimHistory(std::deque<std::unique_ptr<ICommand>>& stack) {
        while (stack.size() > kMaxHistory) {
            stack.pop_front();
        }
    }

    void UndoRedoSystem::Undo() {
        if (mUndoStack.empty()) return;

        auto command = std::move(mUndoStack.back());
        mUndoStack.pop_back();

        command->Undo();

        mRedoStack.push_back(std::move(command));
    }

    void UndoRedoSystem::Redo() {
        if (mRedoStack.empty()) return;

        auto command = std::move(mRedoStack.back());
        mRedoStack.pop_back();

        command->Execute();

        mUndoStack.push_back(std::move(command));
    }
}