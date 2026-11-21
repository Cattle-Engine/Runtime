#pragma once

#include <type_traits>
#include <memory>
#include <deque>

namespace CE::Editor {
    class ICommand {
        public:
            virtual ~ICommand() = default;

            virtual void Execute() = 0;
            virtual void Undo() = 0;
    };

    class UndoRedoSystem {
        public:
            template<typename T, typename... Args>
            void ExecuteCommand(Args&&... args) {
                static_assert(std::is_base_of_v<ICommand, T>, "T must inherit from ICommand");

                std::unique_ptr<T> command = std::make_unique<T>(std::forward<Args>(args)...);

                command->Execute(); 
                mUndoStack.push_back(std::move(command));
                mRedoStack.clear();

                TrimHistory(mUndoStack);
            }

            void Undo();
            void Redo();
        private:
            static constexpr size_t kMaxHistory = 100;

            std::deque<std::unique_ptr<ICommand>> mUndoStack;
            std::deque<std::unique_ptr<ICommand>> mRedoStack;

            void TrimHistory(std::deque<std::unique_ptr<ICommand>>&);
    };
}