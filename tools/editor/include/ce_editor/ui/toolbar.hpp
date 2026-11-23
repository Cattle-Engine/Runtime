#pragma once


namespace CE::Editor {
	class EditorInstance;

	namespace UI {
		// draws an imgui toolbar at the top of the viewport
		class MainViewPortToolBar {
		public:
			MainViewPortToolBar(EditorInstance& instance);

			void Draw();

		private:
			EditorInstance& mInstance;
		};
	}
}
