#include "AboutScreen.h"
#include "Configuration.h"
#include "splashkit.h"
#include <string>
#include <cmath>
#include <vector>
#include <fstream>

#define TITLE_FONT_SIZE 38
#define TITLE_FONT_CHAR_WIDTH 32
#define TITLE_FONT_CHAR_HEIGHT 32
#define TITLE_FONT_Y ((((ARCADE_MACHINE_RES_Y / 2) + (TITLE_FONT_CHAR_HEIGHT / 2))) / 3)
#define STAR_COUNT 1024
#define DISTANCE_SHIFT 10

#define CONTRIBUTION_TIME (60 * 3)
#define CONTRIBUTION_FADE_TIME 30

static const char *title = "About The Thoth Tech Arcade Machine!";
static const char *description[] =  {
	"This arcade machine has been created",
	"by students undertaking capstone units",
	"in the School of Information Technology",
	"as a platform to designed to showcase",
	"games built by students using SplashKit.",
	"",
	"Lines of code"
};

static const std::string createdBy = "Created by";

AboutScreen::AboutScreen() {
	this->m_shouldQuit = false;//初始化退出标志为False
	this->m_titleX = ARCADE_MACHINE_RES_X;//将标题的X位置定位在屏幕最右边
	this->m_title = std::string(title);//将m_title变成string风格的字符串，方便进行各种操作，如lenth()
	this->m_titleEnd = ((TITLE_FONT_CHAR_WIDTH * this->m_title.length()));
	this->m_titleEnd *= -1;
	/*这里的设计涉及到打印逻辑，在打印标题的时候，每打印一个字符，m_titleX的位置就加一个TITLE_FONT_CHAR_WIDTH，所以标题的总长度应为TITLE_FONT_CHAR_WIDTH * this->m_title.length()
	也就是字符长度（m_title.length()）乘以字符宽度（TITLE_FONT_CHAR_WIDTH），而当字符从右往左完全滚动出屏幕之时，第一个字符的X位置应该刚好是-标题总长度，因为此时
	最右边的字符坐标应该为0，所以m_titleEnd的位置应该是-标题总长度=-((TITLE_FONT_CHAR_WIDTH * this->m_title.length()));，也就是37行m_titleEnd的最终值
	*/
	this->m_stars = std::vector<struct s_star>();//初始化数组m_stars
	this->m_contributorsIndex = 0;
	this->m_contributorTicker = 0;
	this->m_ticker = 0;

	for (int i=0; i<STAR_COUNT; ++i) {
		struct s_star star;
		star.x = (rand() % (ARCADE_MACHINE_RES_X - 0 + 1)  + 0);//随机生成星星的X位置
		star.y = (rand() % (ARCADE_MACHINE_RES_Y - 0 + 1) + 0);//随机生成星星的Y位置
		/*这是一个经典限制取值范围的公式：(rand() % (max - min +1)) + min,前面的rand() % (max - min +1)能保证得出的余值范围是0 到 max - min,因为rand()函数取值
		够广够多，则在这种情况下，a % b 的取值范围是[0,b-1]，所以rand() % (ARCADE_MACHINE_RES_X - 0 + 1)的范围是在0到ARCADE_MACHINE_RES_X - 0，再加一个0就是
		0到ARCADE_MACHINE_RES_X，Y方向上的计算同理
		*/
		star.distance = (rand() % (DISTANCE_SHIFT - 1 + 1) + 1);
		//！！！
		double brightness = (rand() % (80 - 40) + 40);
		//同理这个亮度取值范围应该是在40到79
		brightness /= 100;
		//规范brightness的值在0-1之间
		star.c.a = brightness;
		star.c.r = 1;
		star.c.g = 1;
		star.c.b = 1;
		//argb调色，a为我们取的brightness
		this->m_stars.push_back(star);
	}
	//循环生成所有结构体(1024颗)s_star星星的循环，包括每个星星的X,Y位置，亮度，distance，并把这些结构体push进数组m_stars
	
	this->m_contributors = std::vector<std::string>();//初始化数组m_contributors
	std::ifstream contributors("stats" ARCADE_MACHINE_PATH_SEP "contributors.txt");
	//使用ifstream(输入文件流)输入贡献者名单文件 ARCADE_MACHINE_PATH_SEP是为了应对不同系统风格的分隔符
	std::string line;
	while (std::getline(contributors, line)) {
		if (line.length() > 0)
			this->m_contributors.push_back(line);
	}
	contributors.close();
	//按行读取贡献者名单，并把贡献者名字加入到m_contributors数组里
	
	this->m_linesOfCode = std::vector<std::string>();//初始化m_linesOfCode数组
	std::ifstream linesOfCode("stats" ARCADE_MACHINE_PATH_SEP "lines-of-code.txt");
	while (std::getline(linesOfCode, line)) {
		if (line.length() > 0)
			this->m_linesOfCode.push_back(line);
	}
	//按行读取贡献者代码数量文件，并把贡献者代码数量加入到m_linesOfCode数组里
	
	this->m_gitContributions = std::vector<std::string>();
	std::ifstream contributions("stats" ARCADE_MACHINE_PATH_SEP "git.txt");
	while (std::getline(contributions, line)) {
		if (line.length() > 0)
			this->m_gitContributions.push_back(line);
	}
	//同上两个代码块
}
//初始化构造函数

void AboutScreen::onExit() {
	if (music_playing())
		stop_music();
}
//在退出以后关闭音乐播放的函数

void AboutScreen::readInput() {
	if (quit_requested() || key_down(ESCAPE_KEY))
		this->m_shouldQuit = true;
}
//将m_shouldQuit标识改成true的函数(如果用户请求)

void AboutScreen::tick() {
	this->shiftTitle();//更新标题位置
	this->shiftStars();//更新星星位置
	this->tickContributor();

	// Every 1/4 second.
	if (this->m_ticker % 15 == 0) {
		if (! music_playing())
			play_music("music_about");
	}
	//每0.25秒检查一次，因为我们是60fps，15帧近似与0.25秒
	this->m_ticker++;//增加帧数记录
}
//每帧更新函数，包括位置音乐计时

void AboutScreen::render() {
	clear_screen(COLOR_BLACK);//清理屏幕使其成为黑色

	this->renderStars();//渲染星星
	this->renderTitle();//渲染标题
	this->renderDescription();//渲染描述
	this->renderContributor();//渲染贡献者

	refresh_screen();//刷新屏幕展现渲染结果
}
//每调用一次依次执行：清屏，渲染星星，渲染标题，渲染描述，渲染贡献者，刷新屏幕，后渲染者在先渲染者之上
void AboutScreen::shiftTitle() {
	this->m_titleX -= 6;

	if (this->m_titleX < this->m_titleEnd)
		this->m_titleX = ARCADE_MACHINE_RES_X;
}
//每次调用都让m_titleX向左平移6个单位，当m_titleX<m_titleEnd时，让m_titleX重新回到最右边

color AboutScreen::getRainbowShade(double x) {
	color c;
	double rd = (double)ARCADE_MACHINE_RES_X / 16;//红色影响因子，这个值越小，sin(x/rd)变化就越大，所以红色就变得越快，这个值最小
	double gd = (double)ARCADE_MACHINE_RES_X / 10;//绿色影响因子
	double bd = (double)ARCADE_MACHINE_RES_X / 6;//蓝色影响因子，这个值最大
	double p = sin(x / rd) * 0.5;
	double g = sin(x / gd) * 0.5;
	double b = sin(x / bd) * 0.5;
	//将三原色的pgb取值范围处理成[-0.5,0.5]
	c.r = 0.5 + p;
	c.g = 0.5 + g;
	c.b = 0.5 + b;
	//将三原色渲染的变量范围处理成正确的[0,1]，所以最后结果是红色变化得最快，蓝色变化的最慢，所以因为三原色变化速度的差值，会出现彩虹色。
	return c;
}
//通过三原色变化因子的大小和sin函数实现三原色变化速度的差异，所以每次会生成不同颜色并且返回，看起来像彩虹
void AboutScreen::renderTitle() {
	double x = this->m_titleX;//一开始x=this->m_titleX也就是在屏幕最右方

	for (int i=0; i<this->m_title.length(); ++i) {
		double y = TITLE_FONT_Y + (sin(x / 80) * 115);
		//通过sin里带入X值，实现标题y的上下浮动，且因为每个字符的X值不同，所以浮动高度不同，又由于sin函数的特性，会导致看起来像波浪形状，浮动范围在上下115
		double fontSize = TITLE_FONT_SIZE + (sin(x / 120) * 8);
		//同理，但是这里是为了改变字符大小，字符大小波动在上下8
		color c = this->getRainbowShade(x);
		//getRainbowShade(x)通过随机字符颜色
		draw_text(
			this->m_title.substr(i, 1), //单独取每个字符渲染
			c,//字符颜色
			"font_about",//字符字体
			fontSize,//字符大小
			x,//字符X位置
			y//字符Y位置
		);	

		x += TITLE_FONT_CHAR_WIDTH;//两个字符间距
	}
	//(一个一个!)地渲染字符
}
//渲染标题，通过sin函数和getRainbowShade()还有一个一个字渲染的方法实现标题波浪形运行，字体忽大忽小，颜色彩虹的效果

void AboutScreen::shiftStars() {
	for (int i=0; i<this->m_stars.size(); ++i) {
		this->m_stars[i].x += this->m_stars[i].distance;
		if (this->m_stars[i].x > ARCADE_MACHINE_RES_X)
			this->m_stars[i].x = -10;
	}
}
//每次调用shiftStars()时，为每一颗星星的X加上随机数distance，如果加完以后星星超出右边界，则把星星放到-10的位置，然后再飘回来，达到动态效果

void AboutScreen::renderStars() {
	for (auto star : this->m_stars)
		fill_rectangle(star.c, star.x, star.y, star.distance / 2, star.distance / 4);
}
//渲染星星的函数fill_rectangle()最后两项是矩形宽高，所以这个渲染会出现近的星星更大更快，远的星星更小更慢的情况，所以这里的distance是深度/速度等级

void AboutScreen::renderDescription() {
	double offset = sin(((double)this->m_ticker / 16)) * 12;//计算X方向波动的影响因子，范围在-12到12个像素点
	double offsetX = sin((double)this->m_ticker / 24) * 19;//计算Y方向波动的影响因子，范围在-12到12个像素点
	//计算XY方向波动的影响因子
	double y = 480 + offset;//计算Y方向总共的波动值
	double x = 140 + offsetX;//计算X方向总共的波动值
	//引入sin主导的波动影响因子，使得文本描述会周期性波动
	int maxIOffset = 0;
	for (int i=0; i<sizeof(description) / sizeof(description[0]); ++i) {
		maxIOffset = (i * 32);
		draw_text(description[i], COLOR_WHITE, "font_about", 18, x, y + maxIOffset);
	}
	/*这里的description是一个指针数组，存放着每行描述的指针，因此我们用指针数组大小除以指针数组的每个元素大小，得到总的元素个数，也就是描述行数，然后用draw_text()进行渲染
	每隔一行多一个行间距maxIOffset，这也就是maxIOffset乘i的原因，maxIOffset始终为32*/
	
	y = y + maxIOffset + 32;
	for (int i=0; i<this->m_linesOfCode.size(); ++i) {
		maxIOffset = (i * 32);
		draw_text(this->m_linesOfCode[i], COLOR_WHITE, "font_about", 18, x, y + (maxIOffset));
	}
	//在描述的下面间隔32再次渲染linesOfCode的情况，也是每行间隔maxIOffset，32
	y = y + maxIOffset + 64;
	draw_text("Contributions", COLOR_WHITE, "font_about", 18, x, y);
	y += 32;

	for (int i=0; i<this->m_gitContributions.size(); ++i) {
		maxIOffset = (i * 32);
		draw_text(this->m_gitContributions[i], COLOR_WHITE, "font_about", 18, x, y + (maxIOffset));
	}
	//在linesOfCode的下面间隔32再次渲染Contributions的情况，也是每行间隔maxIOffset，32
}
//渲染描述，贡献者代码行数，贡献者情况

void AboutScreen::loop() {
	while (! this->m_shouldQuit) {
		process_events();//调用splashkit.h里的事件处理函数

		this->readInput();//检查有没有申请退出
		this->tick();//更新位置音乐计时器
		this->render();//渲染画面

		delay(1000 / 60);//定下FPS为60
	}

}
//循环函数

void AboutScreen::main() {
	// Clear music and start the about screen music.
	stop_music();
	play_music("music_about");

	this->loop();

	// Tidy up once the loop is done.
	this->onExit();
}
//AboutScreen的入口函数

void AboutScreen::tickContributor() {
	this->m_contributorTicker++;
	if (this->m_contributorTicker >= CONTRIBUTION_TIME) {
		this->m_contributorTicker = 0;//每过180帧重置，然后增加index换下一个人，在FPS为60的情况下它的时间为3s
		this->m_contributorsIndex = (this->m_contributorsIndex + 1) % this->m_contributors.size();//取模防止越界且起到循环轮播效果,比如如果index现在是5，m_contributors size是4，那就取1轮播
	}
}
//实现循环轮播贡献者名字的支撑逻辑，一个3s

void AboutScreen::renderContributor() {
	int r = this->m_contributorTicker % CONTRIBUTION_TIME;//r会始终保持在0-179的范围
	double ratio = 1;//字号透明度
	double fontSize = 32;//字号大小
	double fontRatio = 1;//字号倍率

	if (r < CONTRIBUTION_FADE_TIME) {
		ratio = r / (double)CONTRIBUTION_FADE_TIME;//随着r的增大，ratio会越来越大，也就是越来越亮直到1
		fontRatio = 1 + ((1 - ratio) * 4);//CONTRIBUTION_FADE_TIME是=30帧，也就是淡化阶段，时间为0.5s，随着ratio的增大，字体倍率会越来越小，也就是慢慢变小
		//实现淡入阶段效果，由大而淡变成小而亮
	} else if ((CONTRIBUTION_TIME - r) < CONTRIBUTION_FADE_TIME) {
		ratio = (CONTRIBUTION_TIME - r) / (double)CONTRIBUTION_FADE_TIME;//随着r的增大，ratio会越来越小，也就是亮度越来越小
		fontRatio = ratio;//将字体倍率大小和亮度相等，实现淡出效果
	}

	fontSize = fontSize * fontRatio;//实现字体大小和字体倍率之间的关系控制
	if (ratio > 1.0)
		ratio = 1.0;//限制ratio大小，防止出错

	color c;
	c.r = ratio;
	c.g = ratio;
	c.b = ratio;


	draw_text(this->m_contributors[this->m_contributorsIndex], c, "font_about", fontSize, 1100, 600);

	double x = 0;
	double y = 0;
	fontSize = 24;
	for (int i=0; i<createdBy.length(); ++i) {
		y = sin((this->m_ticker + (i * 4)) / (double)16) * (double)9;//让每个字符Y位置不同，上下9个像素
		x = sin(this->m_ticker / (double) 50) * (double)225;//让每个字符X位置产生变化，左右225个像素
		double actualX = 1300 + x + (i * 28);//计算每个字符的实际位置，间隔28

		fontRatio = sin((actualX + 190) / (double)80) * (double)1.5;//让每个字符大小不一样，1-1.5之间
		if (fontRatio < 1)
			fontRatio = (double)1;//限制fontRatio大小，防止太小

		color c = this->getRainbowShade(actualX - 350);//将字符位置丢给彩虹函数生成彩虹效果
		draw_text(createdBy.substr(i, 1), c, "font_about", (double)24 * fontRatio, actualX, (double)500 + y);//绘制
	}
}//绘制贡献者，彩虹效果，淡入淡出，位置波动