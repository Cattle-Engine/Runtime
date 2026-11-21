#pragma once

#include "ce_runtime/rendering/renderer.hpp"
#include "ce_runtime/rendering/resources/texture_manager.hpp"

namespace CE::Editor {
	class EditorInstance;

	namespace UI {
		// draws an imgui toolbar at the top of the viewport
		class MainViewPortToolBar {
		public:
			MainViewPortToolBar(EditorInstance& instance)
				: mInstance(instance), mRenderer(nullptr), mTextureManager(nullptr) {}

			void Initialise(
				Renderer::IRenderer& renderer,
				Renderer::Resources::TextureManager& texman
			) {
				mRenderer = &renderer;
				mTextureManager = &texman;
			}

			void Draw();

		private:
			EditorInstance& mInstance;
			Renderer::IRenderer* mRenderer;
			Renderer::Resources::TextureManager* mTextureManager;
		};
	}
}
