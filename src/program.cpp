#include "core/sfmlgameclass.hpp"

using namespace core;

auto main() -> int
{
	const int fps = 60;
	const int width = 800;
	const int height = 600;
	const sf::Color clear = {35, 5, 15, 255};
	SfmlGameClass game(SfmlGameSettings{
		fps,
		false,
		width,
		height,
		"My SFML Game",
		clear,
		sf::Style::Titlebar,
		sf::ContextSettings{},
	});
	return SfmlGameClass::RunBareGame(game);
}
