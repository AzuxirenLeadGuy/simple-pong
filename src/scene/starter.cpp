#include "../core/sfmlgameclass.hpp"
#include "../utils/utils.hpp"
#include "scene.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <memory>

struct PhyObj
{
	sf::Vector2f Pos, Vel;
	sf::RectangleShape Shape;
	PhyObj(sf::Vector2f pos, sf::Vector2f size, sf::Vector2f vel, sf::Color color)
		: Pos(pos)
		, Vel(vel)
		, Shape(size)
	{
		SetColor(color);
	}
	void SetColor(sf::Color color)
	{
		Shape.setFillColor(color);
	}
	void Update(sf::Vector2f acc, float friction, float time)
	{
		acc += -friction * Vel;
		Vel += acc;
		Pos += Vel * time;
		Shape.setPosition(Pos);
	}
	void Draw(sf::RenderWindow &window) const
	{
		window.draw(Shape);
	}
};

using namespace core;
using namespace utils;
using KlPtr = std::unique_ptr<KeyListener>;
using PhyPtr = std::unique_ptr<PhyObj>;

enum class State
{
	Playing,
	Paused,
	GameOver
};

namespace scenes
{
class Starter : public SfmlGameClass::AbstractScene
{
	KlPtr listener;
	PhyPtr board, ball;
	sf::IntRect Boundary;
	State state;

  public:
	Starter() = default;
	auto Load(SfmlGameClass &game) -> int override;
	auto PollEvent(SfmlGameClass &game, sf::Event event) -> int override;
	auto Update(SfmlGameClass &game, long time) -> UpdateResult override;
	auto Draw(SfmlGameClass &game, long delta) -> int override;
	auto Destroy(SfmlGameClass &game) -> int override;
	auto Bounce() -> void;
};
} // namespace scenes

using namespace scenes;

auto Starter::Load(SfmlGameClass &game) -> int
{
	auto init = {
		sf::Keyboard::Key::Escape,
		sf::Keyboard::Key::Enter,
		sf::Keyboard::Key::Left,
		sf::Keyboard::Key::Right,
	};
	listener = std::make_unique<KeyListener>(init);
	auto size = game._window.getSize();
	Boundary = sf::IntRect(0, 0, size.x, size.y);
	auto unit = size.x / game.SharedSettings.UnitDivisor;
	ball = std::make_unique<PhyObj>(
		sf::Vector2f((size.x / 2) - (unit / 2), (size.y / 2) - (unit / 2)),
		sf::Vector2f(unit, unit),
		sf::Vector2f(-game.SharedSettings.BallSpeed, -game.SharedSettings.BallSpeed),
		sf::Color::White);
	auto bd_width = unit * game.SharedSettings.BoardWidthUnit;
	board = std::make_unique<PhyObj>(
		sf::Vector2f((size.x / 2) - (bd_width / 2), size.y - unit),
		sf::Vector2f(bd_width, unit),
		sf::Vector2f(),
		sf::Color::Yellow);
	return 0;
}

auto Starter::PollEvent(SfmlGameClass & /*unused*/, sf::Event event) -> int
{
	if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
	{
		listener->Update(event.key.code, event.type == sf::Event::KeyPressed);
	}
	return 0;
}

auto Starter::Bounce() -> void
{
	const auto bll = ball->Shape.getGlobalBounds();
	const auto brd = board->Shape.getGlobalBounds();
	if (bll.left < Boundary.left || bll.left + bll.width > Boundary.left + Boundary.width)
	{
		ball->Vel.x = -ball->Vel.x;
		if (bll.left < Boundary.left)
		{
			ball->Pos.x = Boundary.left;
		}
		else
		{
			ball->Pos.x = Boundary.left + Boundary.width - bll.width;
		}
	}
	if(bll.top < Boundary.top)
	{
		ball->Vel.y = -ball->Vel.y;
		ball->Pos.y = Boundary.top;
	}
	if(bll.intersects(brd))
	{
		ball->Vel.y = -ball->Vel.y;
		ball->Pos.y = brd.top - bll.height;
	}
	if(brd.left < Boundary.left || brd.left + brd.width > Boundary.left + Boundary.width)
	{
		board->Vel.x = -board->Vel.x;
		if(brd.left < Boundary.left)
		{
			board->Pos.x = Boundary.left;
		}
		else
		{
			board->Pos.x = Boundary.left + Boundary.width - brd.width;
		}
	}
}

auto Starter::Update(SfmlGameClass &game, const long time) -> Starter::UpdateResult
{
	bool esc_press = listener->ViewKey(sf::Keyboard::Escape) == KeyState::Just_Release;
	bool ret_press = listener->ViewKey(sf::Keyboard::Enter) == KeyState::Just_Release;
	bool lft_press = listener->ViewKey(sf::Keyboard::Left) == KeyState::Long_Pressed;
	bool rgt_press = listener->ViewKey(sf::Keyboard::Right) == KeyState::Long_Pressed;
	if (esc_press)
	{
		state = State::GameOver;
	}
	sf::Vector2f acc;
	float delta = time / game.SharedSettings.TimeDivisor;
	switch (state)
	{
	case State::Playing:
		if (ret_press)
		{
			board->SetColor(sf::Color::Blue);
			ball->SetColor(sf::Color::Magenta);
			state = State::Paused;
		}
		if (lft_press)
		{
			acc.x = -1;
		}
		else if (rgt_press)
		{
			acc.x = 1;
		}
		board->Update(
			acc * game.SharedSettings.BoardSpeed,
			game.SharedSettings.BoardFriction,
			delta);
		ball->Update(sf::Vector2f(), 0, delta);
		Bounce();
		if(ball->Pos.y > Boundary.top + Boundary.height)
		{
			state = State::GameOver;
		}
		break;
	case State::Paused:
		if (ret_press)
		{
			board->SetColor(sf::Color::Yellow);
			ball->SetColor(sf::Color::White);
			state = State::Playing;
		}
		break;
	case State::GameOver: game.ExitCall(); break;
	}

	listener->FrameEnd();
	return Starter::UpdateResult{0, nullptr};
}

auto Starter::Draw(SfmlGameClass &game /*unused*/, const long /*unused*/) -> int
{
	switch (state)
	{
	case State::Playing: ball->Draw(game._window);
	case State::Paused: board->Draw(game._window);
	case State::GameOver: break;
	}
	return 0;
}

auto Starter::Destroy(SfmlGameClass & /*unused*/) -> int
{
	listener.reset();
	return 0;
}

auto scenes::GetStarter() -> std::unique_ptr<SfmlGameClass::AbstractScene>
{
	return std::make_unique<Starter>();
}