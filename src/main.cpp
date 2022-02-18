/*
* @Author: UnsignedByte
* @Date:   2022-02-17 08:41:41
* @Last Modified by:   UnsignedByte
* @Last Modified time: 2022-02-17 09:35:17
*/

#include <SFML/Graphics.hpp>
// #include <SFML/System.hpp>
#include <cmath>
#include <iostream>
#include <cstring>
#include <cstdio>
#include "solver.hpp"

const int WIDTH = 1200;
const int HEIGHT = 900;
const sf::Time frameTime = sf::seconds(1.f/60.f);

int main()
{
	solver::init();

	// Create the main window
	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Wordless");
	window.setVerticalSyncEnabled(0);
	sf::Clock renderClock;

	// window.setFramerateLimit(60);
	while (window.isOpen())
	{
		// Process events
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch(event.type)
			{
				// Close window: exit
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::KeyPressed:
					solver::charEvent(event.key.code);
			}
		}

		window.clear();

		window.display();

		// std::cout << renderClock.getElapsedTime().asMilliseconds() << std::endl;
		sf::sleep(frameTime-renderClock.getElapsedTime());
		renderClock.restart();
	}

	return 0;
}