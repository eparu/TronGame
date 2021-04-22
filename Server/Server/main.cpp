#include <SFML/Graphics.hpp>
#include <random>
#include <iostream>
#include <utility>
#include <vector>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
using namespace sf;

constexpr int W = 600;
constexpr int H = 480;

constexpr int speed = 1;

class Field
{
public:
    Field()
    {
        cells.assign(W, std::vector<bool>(H));
        for (auto i = 0U; i < W; i++)
        {
            for (auto j = 0U; j < H; j++)
            {
                cells[i][j] = false;
            }
        }
    }
    bool isOccupiedCell(int x, int y)
    {
        return cells[x][y];
    }
    bool isOccupiedCell(std::pair<int, int> point)
    {
        return cells[point.first][point.second];
    }
    void setCell(int x, int y)
    {
        cells[x][y] = true;
    }
    void setCell(std::pair<int, int> point)
    {
        cells[point.first][point.second] = true;
    }
private:
    std::vector<std::vector<bool>> cells;
};

enum class Direction
{
    Down,
    Left,
    Right,
    Up
};

class Player
{
public:
    Player(Color c): color(c)
    {

        std::uniform_int_distribution <> uid_dir(0, 3);

        std::default_random_engine dre(std::chrono::system_clock().now().time_since_epoch().count());
        x = rand() % W;
        y = rand() % H;
        dir = static_cast<Direction>(uid_dir(dre));
    }
    Player(Color c, int p_x, int p_y, Direction d):
    color(c), x(p_x), y(p_y), dir(d) {}
    void update()
    {
        switch (dir) {
            case Direction::Up:
                y--;
                break;
            case Direction::Left:
                x--;
                break;
            case Direction::Right:
                x++;
                break;
            case Direction::Down:
                y++;
                break;
            default:
                break;
        }

        if (x >= W) x = 0;
        if (x < 0)  x = W - 1;
        if (y >= H) y = 0;
        if (y < 0)  y = H - 1;
    }

    std::pair<int, int> getСoordinates()
    {
        return std::pair<int, int>(x, y);
    }
    int getX()
    {
        return x;
    }
    int getY()
    {
        return y;
    }
    void setX(int x_value)
    {
        x = x_value;
    }
    void setY(int y_value)
    {
        y = y_value;
    }
    Direction getDirection()
    {
        return dir;
    }
    void setDirection(Direction d)
    {
        dir = d;
    }
    Color getColor()
    {
        return color;
    }
    ~Player() = default;

private:
    int x, y;
    Direction dir;
    Color color;
};


void read_data_until(boost::asio::ip::tcp::socket& socket, Player& p)
{
    while (true)
    {
        boost::asio::streambuf buffer;

        boost::asio::read_until(socket, buffer, '\n');

        int message;
        std::istream input_stream(&buffer);
        input_stream >> message;

        auto m_dir = static_cast<Direction>(message);
        switch (m_dir) {
            case Direction::Down:
                if (p.getDirection() != Direction::Up)
                {
                    p.setDirection(Direction::Down);
                }
                break;
            case Direction::Left:
                if (p.getDirection() != Direction::Right)
                {
                    p.setDirection(Direction::Left);
                }
                break;
            case Direction::Right:
                if (p.getDirection() != Direction::Left)
                {
                    p.setDirection(Direction::Right);
                }
                break;
            case Direction::Up:
                if (p.getDirection() != Direction::Down)
                {
                    p.setDirection(Direction::Up);
                }
                break;
            default:
                break;
        }
        if (message == 10)
        {
            boost::asio::write(socket, boost::asio::buffer("10\n"));
            return;
        }
    }
}



int main()
{
    Field field;

    RenderWindow window(VideoMode(W, H), "The Tron Game!");
    window.setFramerateLimit(60);

    Texture texture;
    texture.loadFromFile("background.jpg");
    Sprite sBackground(texture);

    Player p1(Color::Red, 100, 100, Direction::Down);
    Player p2(Color::Green, 500, 400, Direction::Up);

    Sprite sprite;
    RenderTexture t;
    t.create(W, H);
    t.setSmooth(true);
    sprite.setTexture(t.getTexture());
    t.clear();
    t.draw(sBackground);

    bool Exit = false;

    const std::size_t size = 30;

    auto port = 8001;

    bool flag = true;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);

    boost::asio::io_service io_service;

    try
    {
        boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());

        acceptor.bind(endpoint);

        acceptor.listen(size);

        boost::asio::ip::tcp::socket socket(io_service);

        acceptor.accept(socket);


        std::thread Thread(read_data_until, std::ref(socket), std::ref(p2));

        while (window.isOpen())
        {
            Event e;
            while (window.pollEvent(e))
            {
                if (e.type == Event::Closed)
                    window.close();
            }


            if (Keyboard::isKeyPressed(Keyboard::Left) &&
                p1.getDirection() != Direction::Right &&
                p1.getDirection() != Direction::Left)
            {
                p1.setDirection(Direction::Left);
                boost::asio::write(socket, boost::asio::buffer("1\n"));
            }
            if (Keyboard::isKeyPressed(Keyboard::Right) &&
                p1.getDirection() != Direction::Left &&
                p1.getDirection() != Direction::Right)
            {
                p1.setDirection(Direction::Right);
                boost::asio::write(socket, boost::asio::buffer("2\n"));
            }
            if (Keyboard::isKeyPressed(Keyboard::Up) &&
                p1.getDirection() != Direction::Down &&
                p1.getDirection() != Direction::Up)
            {
                p1.setDirection(Direction::Up);
                boost::asio::write(socket, boost::asio::buffer("3\n"));
            }
            if (Keyboard::isKeyPressed(Keyboard::Down) &&
                p1.getDirection() != Direction::Up &&
                p1.getDirection() != Direction::Down)
            {
                p1.setDirection(Direction::Down);
                boost::asio::write(socket, boost::asio::buffer("0\n"));
            }

            if (Exit)    continue;

            for(int i = 0;i < speed; i++)
            {
                p1.update(); p2.update();
                if (field.isOccupiedCell(p1.getСoordinates()))
                {
                    Exit = true;
                    std::cout << "Failed\n";
                }
                if (field.isOccupiedCell(p2.getСoordinates()))
                {
                    if (!Exit)
                    {
                        Exit = true;
                        std::cout << "Win\n";
                    }
                }

                field.setCell(p1.getСoordinates());
                field.setCell(p2.getСoordinates());

                CircleShape c(3);
                c.setPosition(p1.getX(),p1.getY());

                c.setFillColor(p1.getColor());
                t.draw(c);
                c.setPosition(p2.getX(), p2.getY());
                c.setFillColor(p2.getColor());
                t.draw(c);
                t.display();
            }

            window.clear();
            window.draw(sprite);
            window.display();
        }
        boost::asio::write(socket, boost::asio::buffer("10\n"));
        Thread.join();
    }
    catch (boost::system::system_error& e)
    {
        std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;

        system("pause");

        return e.code().value();
    }
    std::cout << "end\n";


    return 0;
}
