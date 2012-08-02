#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

#define maxx	64
#define maxy	20

#define max	maxx-2
#define may	maxy-2

#define mix	1
#define miy	1

#define minx	0
#define miny	0

#define SLEEPT		125000

#define ARRAYSIZE(a)	(sizeof(a)/sizeof(a[0]))
#define MAXSNAKELEN	100

#define BODY		"s"

int	end = 0;

struct Position {
	int		x;
	int		y;
};

void setpos(struct Position *p, int x, int y)
{
	p->x = x;
	p->y = y;
}

char *DIRS[] = { "v", "<", ">", "^", NULL };

enum {
	UP,
	RIGHT,
	LEFT,
	DOWN
};

struct Snake {
	struct Position pos[MAXSNAKELEN];
	int		len;
	int		dir;
	int		score;
};

struct Snake snake;

struct Wall {
	struct Position pos[max*may];
	int		len;
};

struct Wall wall;

#define LEN(a)	((sizeof(a)/sizeof(*a)))

char *FOOD[] = {
		"*", "o", "O", "@", "8", "&", "$"
};

enum {
	ASTERISK,
	SMALLO,
	BIGO,
	PERCENT,
	EIGHT,
	AND,
	DOLLAR,
};

struct Food {
	struct Position pos;
	int		val;
};

struct Food food;

static int map_no = 0;

static int total_score = 0;

/* ========================================================================= */

void init_curses()
{
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, FALSE);
    curs_set(0);
}

void init_snake()
{
	snake.len = 6;
	int i;
	for (i = 0; i < snake.len; i++)
	{
		snake.pos[i].x = 10 + i;
		snake.pos[i].y = 10;
	}
	snake.dir = RIGHT;
	snake.score = 0;
}

void end_curses()
{
    flushinp();
    erase();
    refresh();
    endwin();
}

int food_touched()
{
	int i;
	for (i = 0; i < snake.len; i++)
	{
		if (snake.pos[i].x == food.pos.x && snake.pos[i].y == food.pos.y)
			return 1;
	}
	return 0;
}

int food_touched_wall()
{
	int i;
	for (i = 0; i < wall.len; i++)
	{
		if (wall.pos[i].x == food.pos.x && wall.pos[i].y == food.pos.y)
			return 1;
	}
	return 0;
}

/* reversed atoi */
void itoa(int n, char *a)
{
	int i, j, len = 0;
	for (i=0; i < LEN(a); i++)
		a[i] = 0;

	if (n == 0)
	{
		a[0] = '0';
		a[1] = '\0';
		return;
	}

	i = n;
	while (i) {
		len++;
		i /= 10;
	}
	j = len;

	a[j] = 0;
	while (len--) {
		a[--j] = (n % 10) + '0';
		n /= 10;
	}
}

void print_scores()
{
	char text[100];
	char ch[10];

	strcpy(text, "[You scored: ");

	itoa(total_score, ch);

	strcat(text, ch);
	strcat(text, "]");

	mvprintw(maxy/2, (maxx / 2)-(strlen(text)/2), text);
}

void draw_borders();
void draw_snake();
void draw_food();
void draw_walls();
void save_walls();

void snake_finish()
{
	int i;
	for (i = 0; i < 3; i++) {
		draw_borders();
		draw_walls();
		draw_food();
		refresh();
		usleep(SLEEPT*5);
		erase();
		draw_snake();
		refresh();
		usleep(SLEEPT*5);
	}
	erase();
	refresh();
	print_scores();
	refresh();
	usleep(SLEEPT*30);
	end_curses();
}

void self_touched()
{
	// snake touching itself?
	int head = snake.len - 1, i;

	for (i = 0; i < head-1; i++)
	{
		if (snake.pos[head].x == snake.pos[i].x &&
		    snake.pos[head].y == snake.pos[i].y)
		{
			snake_finish();
			//TODO Store user records
			//printf("You scored: %d\n", snake.score);
			exit(0);
		}
	}
}

void init_food()
{
	int x, y;

spawn_food:
	x = random() % max;
	y = random() % may;

	if (x >= max-2)
		x = max-2;
	else
	if (x <= miy)
		x = miy+1;

	if (y >= may-2)
		y = may-2;
	else
	if (y <= miy)
		y = miy+1;

	setpos(&food.pos, x, y);

	if (food_touched() || food_touched_wall())
		goto spawn_food;

	food.val = random() % (LEN(FOOD)-1);
}

void draw_snake()
{
	int i, head = snake.len - 1;
	for (i = 0; i < head; i++)
		mvprintw(snake.pos[i].y, snake.pos[i].x, BODY);

	mvprintw(snake.pos[head].y, snake.pos[head].x, DIRS[snake.dir]);
}

void draw_food()
{
	mvprintw(food.pos.y, food.pos.x, FOOD[food.val]);
}

void move_food()
{
	int x, y;

respawn:
	x = random() % max;
	y = random() % may;

	if (x >= max-2)
		x = max-2;
	else
	if (x <= mix+2)
		x = mix+2;

	if (y >= may-2)
		y = may-2;
	else
	if (y <= miy+2)
		y = miy+2;

	setpos(&food.pos, x, y);

	if (food_touched() || food_touched_wall())
		goto respawn;

	food.val = random() % LEN(FOOD);
}

void do_score()
{
	snake.score += 5 * (food.val+1);
	total_score += snake.score;
}

void wall_touched();

void move_snake()
{
	int head, x, y, xv, yv;
	int grow = 0;
	int i;

	head = snake.len - 1;

	if (food_touched())
	{
		move_food();
		draw_food();
		refresh();
		do_score();
		grow = 1;
	}

	// is gameover?
	self_touched();
	wall_touched();

move_head:
	x = snake.pos[head].x;
	y = snake.pos[head].y;

	xv = 0;
	yv = 0;

	switch (snake.dir)
	{
	case RIGHT:
		xv = 1;
		break;
	case UP:
		yv = -1;
		break;
	case DOWN:
		yv = +1;
		break;
	case LEFT:
		xv = -1;
		break;
	}

	x += xv;
	y += yv;

	if (x > max)
		x = mix;
	else if (x < mix)
		x = max;

	if (y > may)
		y = miy+1;
	else if (y <= miy)
		y = may;

	setpos(&snake.pos[head], x, y);

	if (grow) {
		snake.len++;
		head++;
		setpos(&snake.pos[head], x, y);
		grow = 0;
		goto move_head;
	}

	for (i = 0; i < head; i++)
	{
		snake.pos[i].x = snake.pos[i+1].x;
		snake.pos[i].y = snake.pos[i+1].y;

		if (snake.pos[i].x > max)
			snake.pos[i].x = mix;
		else
		if (snake.pos[i].x < mix)
			snake.pos[i].x = max-1;

		if (snake.pos[i].y > may)
			snake.pos[i].y = miy+1;
		else
		if (snake.pos[i].y <= miy)
			snake.pos[i].y = may;
	}
}

void gpause()
{
	char *text = "[Paused (p to resume)]";
	while (1) {
		mvprintw(maxy / 2, (maxx / 2)-(strlen(text)/2), text);
		refresh();
		int c = getchar();
		if (c == 'p' || c == 'P')
			break;
	}
	erase();
	refresh();
}

void clear_wall(int x, int y)
{
	int i, j, wx, wy, p = -1;

	int px[1000];
	int py[1000];

	for (j = 0, i = 0; i < wall.len; i++)
	{
		wy = wall.pos[i].y;
		wx = wall.pos[i].x;

		if (wx == x && wy == y) {
			p = i;
			continue;
		}

		px[j] = wx;
		py[j] = wy;
		j++;
	}

	if (p < 0)
		return;

	for (i = 0; i < j; i++)
	{
		wall.pos[i].x = px[i];
		wall.pos[i].y = py[i];
	}

	if (wall.len-1 < 0)
		return;

	wall.len--;
}

void append_wall(int x, int y)
{
	++wall.len;

	int i = wall.len-1;

	setpos(&wall.pos[i], x, y);
}

void edit_walls()
{
	int curx, cury;
	cury = maxy / 2;
	curx = maxx / 2;
	while (1) {
		draw_borders();
		draw_walls();
		mvprintw(cury, curx, "O");
		refresh();
		int c = getchar();
		switch (c)
		{
		case 'q':
		case 'e':
		case 27:
			return;
		case 'w':
			cury += (cury > miy+1 ? -1 : 0);
			break;
		case 's':
			cury -= (cury < may ? -1 : 0);
			break;
		case 'd':
			curx += (curx < max ? 1 : 0);
			break;
		case 'a':
			curx -= (curx > mix ? 1 : 0);
			break;
		case 'u':
			--wall.len;
			break;
		case 'x':
			clear_wall(curx, cury);
			break;
		case ' ':
			append_wall(curx, cury);
			break;
		case 'p':
			save_walls();
			break;
		}
		erase();
	}
	erase();
	refresh();
}

void load_walls()
{
	FILE	*fp;
	char	line[1000];
	int	i, y, x, j;

	wall.len = 0, j = 0;

	map_no++;

	char ch[10];
	itoa(map_no, ch);
	strcat(ch, ".map");

	fp = fopen(ch, "r");
	if (fp == NULL)
		return;

	while (fgets(line, sizeof(line), fp)) {
		y = atoi(line);
		i = 0;
		while (line[i++] != ' ')
			;
		x = atoi(line + i);
		setpos(&wall.pos[j], x, y);
		wall.len = j++;
	}

	fclose(fp);
}

void draw_walls()
{
	int i;
	for (i = 0; i < wall.len; i++)
			mvprintw(wall.pos[i].y, wall.pos[i].x, "#");
}

void save_walls()
{
	int i;
	//map_no++;
	char ch[10];
	itoa(map_no, ch);
	strcat(ch, ".map");

	FILE *fp = fopen(ch, "w");

	for (i = 0; i < wall.len; i++)
		fprintf(fp, "%d %d\n", wall.pos[i].y, wall.pos[i].x);

	fclose(fp);
}

void wall_touched()
{
	if (!wall.len)
		return;
	int i, j;
	for (i = 0; i < wall.len; i++)
	{
		for (j = 0; j < snake.len; j++)
		{
			if (snake.pos[j].y == wall.pos[i].y &&
			    snake.pos[j].x == wall.pos[i].x)
			{
				snake_finish();
				exit(0);
			}
		}
	}
}

void draw_borders()
{
	int i;

	/* Horizontal */
	mvprintw(1, 0, "+");
	for (i = 1; i < maxx-1; i++)
		mvprintw(1, i, "--");
	mvprintw(1, maxx-1, "+");

	mvprintw(maxy-1, 0, "+");
	for (i = 1; i < maxx-1; i++)
		mvprintw(maxy-1, i, "--");
	mvprintw(maxy-1, maxx-1, "+");

	/* Vertical */
	for (i = 2; i < maxy-2; i++)
		mvprintw(i, 0, "|");
	mvprintw(maxy-2, 0, "|");

	for (i = 2; i < maxy-2; i++)
		mvprintw(i, maxx-1, "|");
	mvprintw(maxy-2, maxx-1, "|");
}

void draw_info()
{
	char info[100];
	char res[10];
	int n = snake.score;
	itoa(n, res);

	strcpy(info, "[score: ");
	strcat(info, res);
	strcat(info, "]");
	
	strcat(info, "]; head at: ");

	int x = snake.pos[snake.len-1].x;
	int y = snake.pos[snake.len-1].y;

	itoa(y, res);
	strcat(info, res);
	strcat(info, ", ");

	itoa(x, res);
	strcat(info, res);

	strcat(info, "; food at: ");

	x = food.pos.x;
	y = food.pos.y;

	itoa(y, res);
	strcat(info, res);
	strcat(info, ", ");

	itoa(x, res);
	strcat(info, res);

	mvprintw(0, 1, info);
}

void userctl(int key)
{
	switch (key)
	{
	/* Quit game */
	case 27:
		end = 1;
		break;
	/* Pause game */
	case 'p':
		gpause();
		break;
	case 'e':
		edit_walls();
		break;
	case 'w':
		snake.dir = UP;
		break;
	case 'd':
		snake.dir = RIGHT;
		break;
	case 'a':
		snake.dir = LEFT;
		break;
	case 's':
		snake.dir = DOWN;
		break;
	}
}

/* ========================================================================= */

int main()
{
	unsigned int seed = time(NULL);
	srandom(seed);

	init_curses();
	init_snake();
	init_food();

	load_walls();

	while (!end)
	{
		erase();
		draw_borders();
		//draw_info();
		draw_walls();
		draw_snake();
		draw_food();
		move_snake();
		refresh();
		if (snake.score > map_no * 300) {
			load_walls();
			init_snake();
		}
		userctl(getch());
		usleep(SLEEPT);
	}

	end_curses();
	return 0;
}

