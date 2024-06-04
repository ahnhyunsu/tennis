////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>

#ifdef SFML_SYSTEM_IOS
#include <SFML/Main.hpp>
#endif

std::string resourcesDir()
{
#ifdef SFML_SYSTEM_IOS
    return "";
#else
    return "resources/";
#endif
}

////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
////////////////////////////////////////////////////////////
int main()
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    // Define some constants
    const float pi = 3.14159f;
    const float gameWidth = 800;
    const float gameHeight = 600;
    sf::Vector2f paddleSize(25, 100);
    float ballRadius = 10.f;

    // Create the window of the application
    sf::RenderWindow window(sf::VideoMode(static_cast<unsigned int>(gameWidth), static_cast<unsigned int>(gameHeight), 32), "SFML Tennis",
        sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    // Load the sounds used in the game
    sf::SoundBuffer ballSoundBuffer;
    if (!ballSoundBuffer.loadFromFile(resourcesDir() + "ball.wav"))
        return EXIT_FAILURE;
    sf::Sound ballSound(ballSoundBuffer);

    // Create the SFML logo texture:
    sf::Texture sfmlLogoTexture;
    if (!sfmlLogoTexture.loadFromFile(resourcesDir() + "sfml_logo.png"))
        return EXIT_FAILURE;
    sf::Sprite sfmlLogo;
    sfmlLogo.setTexture(sfmlLogoTexture);
    sfmlLogo.setPosition(170, 50);

    // Create the left paddle
    sf::RectangleShape leftPaddle;
    leftPaddle.setSize(paddleSize - sf::Vector2f(3, 3));
    leftPaddle.setOutlineThickness(3);
    leftPaddle.setOutlineColor(sf::Color::Black);
    leftPaddle.setFillColor(sf::Color(100, 100, 200));
    leftPaddle.setOrigin(paddleSize / 2.f);

    // Create the right paddle
    sf::RectangleShape rightPaddle;
    rightPaddle.setSize(paddleSize - sf::Vector2f(3, 3));
    rightPaddle.setOutlineThickness(3);
    rightPaddle.setOutlineColor(sf::Color::Black);
    rightPaddle.setFillColor(sf::Color(200, 100, 100));
    rightPaddle.setOrigin(paddleSize / 2.f);

    // Create the ball
    sf::CircleShape ball;
    ball.setRadius(ballRadius - 3);
    ball.setOutlineThickness(2);
    ball.setOutlineColor(sf::Color::Black);
    ball.setFillColor(sf::Color::White);
    ball.setOrigin(ballRadius / 2, ballRadius / 2);

    // Load the text font
    sf::Font font;
    if (!font.loadFromFile(resourcesDir() + "tuffy.ttf"))
        return EXIT_FAILURE;

    // Initialize the pause message
    sf::Text pauseMessage;
    pauseMessage.setFont(font);
    pauseMessage.setCharacterSize(40);
    pauseMessage.setPosition(170.f, 200.f);
    pauseMessage.setFillColor(sf::Color::White);

#ifdef SFML_SYSTEM_IOS
    pauseMessage.setString("Welcome to SFML Tennis!\nTouch the screen to start the game.");
#else
    pauseMessage.setString("Welcome to SFML Tennis!\n\nPress space to start the game.");
#endif

    // Initialize the score
    int leftScore = 0;
    int rightScore = 0;
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(30);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(gameWidth / 2.f - 50.f, 10.f);
    scoreText.setString("0 : 0");

    // Define the paddles properties
    sf::Clock AITimer;
    const sf::Time AITime = sf::seconds(0.1f);
    const float basePaddleSpeed = 400.f;
    const float boostedPaddleSpeed = 600.f;
    float paddleSpeed = basePaddleSpeed;
    float rightPaddleSpeed = 0.f;
    const float baseBallSpeed = 400.f;
    const float boostedBallSpeed = 600.f;
    float ballSpeed = baseBallSpeed;
    float ballAngle = 0.f; // to be changed later

    sf::Clock clock;
    sf::Clock blockTimer; // Timer for block creation
    bool isPlaying = false;
    bool isSpeedBoosted = false; // New variable to track speed boost
    std::vector<sf::RectangleShape> blocks; // Vector to store blocks

    while (window.isOpen())
    {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Window closed or escape key pressed: exit
            if ((event.type == sf::Event::Closed) ||
                ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
            {
                window.close();
                break;
            }

            // Space key pressed: play or speed boost
            if (((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Space)) ||
                (event.type == sf::Event::TouchBegan))
            {
                if (!isPlaying)
                {
                    // (re)start the game
                    isPlaying = true;
                    clock.restart();
                    blockTimer.restart(); // Reset the block timer
                    blocks.clear(); // Clear all existing blocks

                    // Reset the position of the paddles and ball
                    leftPaddle.setPosition(10.f + paddleSize.x / 2.f, gameHeight / 2.f);
                    rightPaddle.setPosition(gameWidth - 10.f - paddleSize.x / 2.f, gameHeight / 2.f);
                    ball.setPosition(gameWidth / 2.f, gameHeight / 2.f);

                    // Reset the ball angle
                    do
                    {
                        // Make sure the ball initial angle is not too much vertical
                        ballAngle = static_cast<float>(std::rand() % 360) * 2.f * pi / 360.f;
                    } while (std::abs(std::cos(ballAngle)) < 0.7f);
                }
                else
                {
                    // Toggle speed boost
                    isSpeedBoosted = !isSpeedBoosted;
                    paddleSpeed = isSpeedBoosted ? boostedPaddleSpeed : basePaddleSpeed;
                    ballSpeed = isSpeedBoosted ? boostedBallSpeed : baseBallSpeed;
                }
            }

            // Window size changed, adjust view appropriately
            if (event.type == sf::Event::Resized)
            {
                sf::View view;
                view.setSize(gameWidth, gameHeight);
                view.setCenter(gameWidth / 2.f, gameHeight / 2.f);
                window.setView(view);
            }
        }

        if (isPlaying)
        {
            float deltaTime = clock.restart().asSeconds();

            // Move the player's paddle
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) &&
                (leftPaddle.getPosition().y - paddleSize.y / 2 > 5.f))
            {
                leftPaddle.move(0.f, -paddleSpeed * deltaTime);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) &&
                (leftPaddle.getPosition().y + paddleSize.y / 2 < gameHeight - 5.f))
            {
                leftPaddle.move(0.f, paddleSpeed * deltaTime);
            }

            if (sf::Touch::isDown(0))
            {
                sf::Vector2i pos = sf::Touch::getPosition(0);
                sf::Vector2f mappedPos = window.mapPixelToCoords(pos);
                leftPaddle.setPosition(leftPaddle.getPosition().x, mappedPos.y);
            }

            // Move the computer's paddle
            if (((rightPaddleSpeed < 0.f) && (rightPaddle.getPosition().y - paddleSize.y / 2 > 5.f)) ||
                ((rightPaddleSpeed > 0.f) && (rightPaddle.getPosition().y + paddleSize.y / 2 < gameHeight - 5.f)))
            {
                rightPaddle.move(0.f, rightPaddleSpeed * deltaTime);
            }

            // Update the computer's paddle direction according to the ball position
            if (AITimer.getElapsedTime() > AITime)
            {
                AITimer.restart();
                if (ball.getPosition().y + ballRadius > rightPaddle.getPosition().y + paddleSize.y / 2)
                    rightPaddleSpeed = paddleSpeed;
                else if (ball.getPosition().y - ballRadius < rightPaddle.getPosition().y - paddleSize.y / 2)
                    rightPaddleSpeed = -paddleSpeed;
                else
                    rightPaddleSpeed = 0.f;
            }

            // Move the ball
            float factor = ballSpeed * deltaTime;
            ball.move(std::cos(ballAngle) * factor, std::sin(ballAngle) * factor);

#ifdef SFML_SYSTEM_IOS
            const std::string inputString = "Touch the screen to restart.";
#else
            const std::string inputString = "Press space to restart or\nescape to exit.";
#endif

            // Check collisions between the ball and the screen
            if (ball.getPosition().x - ballRadius < 0.f)
            {
                isPlaying = false;
                rightScore++;
                if (rightScore >= 5)
                {
                    pauseMessage.setString("You Lost the Game!\n\n" + inputString);
                }
                else
                {
                    pauseMessage.setString("You Lost!\n\n" + inputString);
                }
                scoreText.setString(std::to_string(leftScore) + " : " + std::to_string(rightScore));
            }
            if (ball.getPosition().x + ballRadius > gameWidth)
            {
                isPlaying = false;
                leftScore++;
                if (leftScore >= 5)
                {
                    pauseMessage.setString("You Won the Game!\n\n" + inputString);
                }
                else
                {
                    pauseMessage.setString("You Won!\n\n" + inputString);
                }
                scoreText.setString(std::to_string(leftScore) + " : " + std::to_string(rightScore));
            }
            if (ball.getPosition().y - ballRadius < 0.f)
            {
                ballSound.play();
                ballAngle = -ballAngle;
                ball.setPosition(ball.getPosition().x, ballRadius + 0.1f);
            }
            if (ball.getPosition().y + ballRadius > gameHeight)
            {
                ballSound.play();
                ballAngle = -ballAngle;
                ball.setPosition(ball.getPosition().x, gameHeight - ballRadius - 0.1f);
            }

            // Check the collisions between the ball and the paddles
            // Left Paddle
            if (ball.getPosition().x - ballRadius < leftPaddle.getPosition().x + paddleSize.x / 2 &&
                ball.getPosition().x - ballRadius > leftPaddle.getPosition().x &&
                ball.getPosition().y + ballRadius >= leftPaddle.getPosition().y - paddleSize.y / 2 &&
                ball.getPosition().y - ballRadius <= leftPaddle.getPosition().y + paddleSize.y / 2)
            {
                if (ball.getPosition().y > leftPaddle.getPosition().y)
                    ballAngle = pi - ballAngle + static_cast<float>(std::rand() % 20) * pi / 180;
                else
                    ballAngle = pi - ballAngle - static_cast<float>(std::rand() % 20) * pi / 180;

                ballSound.play();
                ball.setPosition(leftPaddle.getPosition().x + ballRadius + paddleSize.x / 2 + 0.1f, ball.getPosition().y);
            }

            // Right Paddle
            if (ball.getPosition().x + ballRadius > rightPaddle.getPosition().x - paddleSize.x / 2 &&
                ball.getPosition().x + ballRadius < rightPaddle.getPosition().x &&
                ball.getPosition().y + ballRadius >= rightPaddle.getPosition().y - paddleSize.y / 2 &&
                ball.getPosition().y - ballRadius <= rightPaddle.getPosition().y + paddleSize.y / 2)
            {
                if (ball.getPosition().y > rightPaddle.getPosition().y)
                    ballAngle = pi - ballAngle + static_cast<float>(std::rand() % 20) * pi / 180;
                else
                    ballAngle = pi - ballAngle - static_cast<float>(std::rand() % 20) * pi / 180;

                ballSound.play();
                ball.setPosition(rightPaddle.getPosition().x - ballRadius - paddleSize.x / 2 - 0.1f, ball.getPosition().y);
            }

            // Create a new block every 10 seconds
            if (blockTimer.getElapsedTime().asSeconds() >= 10.f)
            {
                sf::RectangleShape block;
                block.setSize(sf::Vector2f(100, 20));
                block.setFillColor(sf::Color::Green);
                block.setPosition(std::rand() % static_cast<int>(gameWidth - 100), std::rand() % static_cast<int>(gameHeight - 20));
                blocks.push_back(block);
                blockTimer.restart();
            }

            // Check collisions between the ball and the blocks
            for (auto& block : blocks)
            {
                if (ball.getGlobalBounds().intersects(block.getGlobalBounds()))
                {
                    ballAngle = -ballAngle;
                    ballSound.play();
                }
            }

            // Check if the game is over
            if (leftScore >= 5 || rightScore >= 5)
            {
                isPlaying = false;
                pauseMessage.setString("Game Over!\n\n" + inputString);
            }
        }

        // Clear the window
        window.clear(sf::Color(50, 50, 50));

        if (isPlaying)
        {
            // Draw the paddles and the ball
            window.draw(leftPaddle);
            window.draw(rightPaddle);
            window.draw(ball);

            // Draw all the blocks
            for (const auto& block : blocks)
            {
                window.draw(block);
            }
        }
        else
        {
            // Draw the pause message
            window.draw(pauseMessage);
            window.draw(sfmlLogo);
        }

        // Draw the score
        window.draw(scoreText);

        // Display things on screen
        window.display();
    }

    return EXIT_SUCCESS;
}
