#ifndef ARCADE_MACHINE_ABOUT_SCREEN_H
#define ARCADE_MACHINE_ABOUT_SCREEN_H
#include <string>
#include <vector>
#include "splashkit.h"

struct s_star {
	double x;
	double y;
	double distance;
	color c;
};//定义结构体星星

class AboutScreen {
private:
	bool m_shouldQuit;//退出标识
	unsigned long long m_ticker;//节拍器
	std::string m_title;//标题
	double m_titleX;//标题横向位置
	double m_titleEnd;//标题结束横向位置
	std::vector<struct s_star> m_stars;//m_stars是一个自动变长且存储着结构体s_star的数组
	int m_contributorsIndex;//贡献者索引
	int m_contributorTicker;//贡献者节拍器
	std::vector<std::string> m_contributors;//存储着贡献者名字的m_contributors
	std::vector<std::string> m_linesOfCode;//存储着贡献者代码数量的m_linesOfCode
	std::vector<std::string> m_gitContributions;

	void readInput();
	void tick();
	void shiftTitle();
	void shiftStars();
	void render();
	color getRainbowShade(double x);
	void renderTitle();
	void renderStars();
	void renderDescription();

	void tickContributor();
	void renderContributor();
	void onExit();
	void loop();
//声明函数方法
public:
	AboutScreen();
	void main();
};

#endif

