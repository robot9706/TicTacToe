#include <stdio.h>   
#include <time.h>    
#include <termios.h> 
#include <unistd.h>  
#include <assert.h>  
#include <string.h>  
#include <stdlib.h>

#define bool int
#define true 1
#define false 0

//#1 Billentyű olvasás (getchar.zip)
typedef enum {
	RegularKey,
	ArrowKey,
	OtherKey,
	NoKey

} KeyType;

typedef enum {
	Up,
	Down,
	Left,
	Right
} Arrow;

typedef struct {
	KeyType keyType;
	union {
		Arrow arrow;
		char c;
	} key;
} KeyCode;

int __getch()
{
	int c = 0;
	
	struct termios org_opts, new_opts;
	int res = 0;
	
	res = tcgetattr(STDIN_FILENO, &org_opts);
	assert(res == 0);
	
	memcpy(&new_opts, &org_opts, sizeof(new_opts));
	new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
	c = getchar();
	
	res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
	assert(res == 0);
	
	return c;
}

KeyCode getKeyCode()
{
	KeyCode ch;
	int c = __getch();
	
	if(c == EOF)
	{
		ch.keyType = NoKey;
		return ch;
	}

	if(c != 27){
		ch.keyType = RegularKey;
		ch.key.c = c;
	}else{
		__getch();
		c = __getch();
		switch(c){
			case 65:
				ch.keyType = ArrowKey;
				ch.key.arrow = Up;
				break;
			case 66:
				ch.keyType = ArrowKey;
				ch.key.arrow = Down;
				break;
			case 67:
				ch.keyType = ArrowKey;
				ch.key.arrow = Right;
				break;
			case 68:
				ch.keyType = ArrowKey;
				ch.key.arrow = Left;
				break;
			default:
				ch.keyType = OtherKey;
				break;
		}
	}
	
	return ch;
}

//#2 Általános eszközök
//ANSI Escape kódok: http://wikipedia.org/wiki/ANSI_escape_code
void cmdClear() //Törli a terminál tartalmát
{
	printf("\e[2J");
}

void cmdCursor(int x, int y) //Mozgatja a kurzort a terminálban
{
	//Terminál számozás 1-től indul, viszont x-et és y-t 0-val indítom a kódban -> +1 kell
	//Ha kisebb mint 1 akkor problémák adódnak
	if(x < -1) x = -1;
	if(y < -1) y = -1;

	//"ESC [Y;XH", számozás 1-től indul
	printf("\e[%d;%dH", y+1, x+1);
}

//Terminál színkódok (bár ez termináltól függ)
typedef enum {
	black = 0,
	red = 1,
	green = 2,
	yellow = 3,
	blue = 4,
	magenta = 5,
	cyan = 6,
	gray = 7
} color;

void cmdTextColor(color c) //Betűszínt állít
{
	printf("\e[3%dm", c);
}

void cmdBgColor(color c) //Betűk háttérszínét állítja
{
	printf("\e[4%dm", c);
}

void cmdResetColors() //A színeket alaphelyzetbe állítja
{
	printf("\e[0m");
}

void cmdCursorBlink(bool on) //Terminálon a kurzor villogásának ki/be kapcsolja
{
	printf("\e[?25%c", (on ? 'h' : 'l'));
}

int randomRange(int min, int max) //Random értéked ad min és max között
{
	return (rand() % (max - min + 1)) + min;
}

//#3 Rajz eszközök
//Játék tábla bal felső sarka
#define TABLE_BASE_X 0
#define TABLE_BASE_Y 0
#define TABLE_SIZE (1+3*6) //Jáétktábla mérete, 3x3 cella (négyzet így nem kell szélesség, magasság külön), 5x5 karakter 1 cella, 1 karakter elválasztó a cellák között + 1 karakter keret baloldalon és felül -> 3*6+1

//Játékos indexek
#define PX 0 //X játékos
#define PO 1 //O játékos
#define PN 2 //Üres

//Színek
#define COLOR_NO red
#define COLOR_OK green

//2D-s pont
typedef struct {
	int x;
	int y;
} point;

point getTilePos(int tx, int ty) //Visszaadja a tx és ty cella helyén a terminál karakter helyét (a cella bal felső sarkát)
{
	point p;
	p.x = TABLE_BASE_X + 1 + tx * 6;
	p.y = TABLE_BASE_Y + 1 + ty * 6;
	return p;
}

char _players[3][5][6] = //2 "játékos" + ("törlés"), 5 sor/játékos, 5 karakter/sor (+'\0')
{
	{
		"X   x",
		" X X ",
		"  X  ",
		" X X ",
		"X   x"
	},
	{
		" OOO ",
		"O   O",
		"O   O",
		"O   O",
		" OOO "
	},
	{
		"     ",
		"     ",
		"     ",
		"     ",
		"     "		
	}
};

void drawPlayer(point p, int id) //Kirajzol egy játékost, "id" a "_players" tömböt indexeli, "p" pedig megadja a rajzolás bal felső sarkát
{
	int i;
	for(i = 0;i<5;i++) //5 sor
	{
		cmdCursor(p.x, p.y + i); //Kurzor a megfelelő helyres
		printf("%s", &_players[id][i][0]); //Megfelelő játékos képének az i. sorának a pointere
	}
}

//Rajzol egy négyzetet ami körbevesz egy cellát a táblán
//"p": megadja a cella bal felső sarkát, "color" a keret színe
void drawOutline(point p, int color)
{
	cmdBgColor(color); //Szín beállítása
	
	int sx = p.x - 1; //p.x a cella bal felső sarka, így p.x-1 adja meg a keret x pozícióját
	int sy = p.y - 1;
	
	int i;
	for(i = 0; i <= 6; i++) //Minden cella 5 karakter széles, így 7 karakter a cella
	{
		cmdCursor(sx, sy + i); //Cella bal oldala
		printf(" ");
		
		cmdCursor(sx + 6, sy + i); //Cella jobb oldala
		printf(" ");
		
		cmdCursor(sx + i, sy); //Cella teteje
		printf(" ");
		
		cmdCursor(sx + i, sy + 6); //Cella alja
		printf(" ");
	}
	
	cmdResetColors();
}

//#4.1 TicTacToe játék
int tiles[3][3]; //3x3-as mátrix ami a cellák tartalmát tárolja (PN: üres, PX: X, PO: O)

void printInfoText() //Kiírja játék közben az irányítást a tábla jobb széléhez képes 3 karakterrel
{
	cmdCursor(TABLE_BASE_X + TABLE_SIZE + 3, 4);
	printf("Irányítás:");
	
	cmdCursor(TABLE_BASE_X + TABLE_SIZE+ 3, 5);
	printf("   Nyilak: Kurzor mozgatsa."); //Szóközökkel a ":"-ok egy oszlopban lesznek
	
	cmdCursor(TABLE_BASE_X + TABLE_SIZE + 3, 6);
	printf("    Enter: Választás.");
	
	cmdCursor(TABLE_BASE_X + TABLE_SIZE + 3, 7);
	printf("      Q/E: Visza a menübe");
}

void resetTiles() //A tábla minden celláját üresre állítja
{
	int x, y;

	for(x = 0;x<3;x++)
		for(y = 0;y<3;y++)
			tiles[x][y] = PN;
}

//#4.2 Számítógépes ellenfél logika
int botCheckLine(int line, int player) //Megnézi, hogy a "line" által megadott sorban van-e 2 "player" cella és egy üres cella
{
	int x = -1; //Az olszlopban az üres cella hely
	int db = 0; //Hány darab "player" cella van a sorban
	
	int i;
	for (i = 0; i < 3; i++) //3 cella / sor
	{
		if(tiles[i][line] == player)
		{
			db++;
		}
		else if(tiles[i][line] == PN) //Üres?
		{
			x = i;
		}
	}
	
	if(db != 2)
	{
		return -1; //Nincs 2db játékos cella, így a lyuk nem számít
	}
	
	return x; //Visszatér azzal az oszloppal, ahol a lyuk van
}

int botCheckColumn(int column, int player) //Megnézi, hogy a "column" által megadott oszlopban van-e "player" cella és egy üres cella
{
	int y = -1; //Az olszlopban az üres cella hely
	int db = 0; //Hány darab "player" cella van a sorban
	
	int i;
	for(i = 0;i<3;i++)
	{
		if(tiles[column][i] == player)
		{
			db++;
		}
		else if(tiles[column][i] == PN) //Üres?
		{
			y = i;
		}
	}
	
	if(db != 2)
	{
		return -1; //Nincs 2db játékos cella, így a lyuk nem számít
	}
	
	return y; //Visszatér a sorral, ahol a lyuk van
}

point botCheckDiagonal(int player) //Megnézi, hogy a táblán a 2 átló valamelyikében van-e 2 "player" cella és egy lyuk
{
	point p; //Pont ahol a lyuk van -> (-1;-1) = nincs lyuk
	p.x = -1;
	p.y = -1;
	
	int dbA = 0; //A: Balfent -> jobblent átló; "player" cellák száma
	int dbB = 0; //B: Jobbfent -> ballent átló; "player" cellák száma
	
	int aXY = -1; //"A" átló üres cellájának X és Y-ja
	int bX = -1; //"B" átló üres cellájának X-e
	int bY = -1; //"B" átló üres cellájának Y-ja
	
	int i;
	for(i = 0; i < 3; i++)
	{
		//A átló
		if(tiles[i][i] == player)
		{
			dbA++;
		}
		else if(tiles[i][i] == PN)
		{
			aXY = i;
		}
			
		//B áltó
		if(tiles[2-i][i] == player)
		{
			dbB++;
		}
		else if(tiles[2-i][i] == PN)
		{
			bX = 2 - i;
			bY = i;
		}
	}
	
	if(dbA == 2 && aXY != -1) //Ha "A" átlóban megvan a 2 játékos cella és van benne lyuk
	{
		p.x = p.y = aXY;
		return p;
	}
	
	if(dbB == 2 && bX != -1 && bY != -1) //Ha "B" átlóban megvan a 2 játékos cella és van benne lyuk
	{
		p.x = bX;
		p.y = bY;
		return p;
	}
	
	return p;
}

point botEmptyForPlayer(int player) //Az előző 3 függvény segítségével megállapítja, hogy a "player" játékosnak van-e olyan cellája amivel nyerhet
{
	int i, x, y;
	point p; //Cella ahova a nyerő lépést kell helyezni
	for(i = 0; i < 3; i++) //3sor, 3oszlop
	{
		x = botCheckLine(i, player);
		if(x != -1) //Ha i. sorban van olyan "x" ahol lyuk van
		{
			p.x = x;
			p.y = i;
			return p; //Itt van egy nyerő lépés
		}
		
		y = botCheckColumn(i, player);
		if(y != -1) //Ha i. oszlopban van olyan "y" ahol lyuk van
		{
			p.x = i;
			p.y = y;
			return p; //Itt van egy nyerő lépés
		}
	}
	
	//Átlók
	p = botCheckDiagonal(player);
	if(p.x != -1 && p.y != -1) //Ha valamelyik átlóban van lyuk
		return p;
	
	p.x = -1;
	p.y = -1;
	return p; //Nincs nyerő lépés
}

//Függvény amely eldönti a számítógép következő lépését
//"botPlayer" a számíótgép játékosa (PX vagy PO)
point botStep(int botPlayer)
{
	int player = (botPlayer == PX ? PO : PX); //számítógép játékosa alapján a játékos
	
	//Van olyan lépés amivel a bot nyerhet?
	point botWin = botEmptyForPlayer(botPlayer);
	if(botWin.x != -1 && botWin.y != -1) //Van
	{
		//Nyerő lépés
		return botWin;
	}
	
	//Van olyan lépés amivel az ellenség nyerhet?
	point enemyWin = botEmptyForPlayer(player);
	if(enemyWin.x != -1 && enemyWin.y != -1) //Van
	{
		//Direkt odalépek >:)
		return enemyWin;
	}
	
	//Nincs nyerő lépés és olyan se amivel az emberi játékost akadályozná -> random lépés
	point step;
	do
	{
		step.x = randomRange(0, 2);
		step.y = randomRange(0, 2);
	}
	while(tiles[step.x][step.y] != PN); //Addig keresek random cellát amég üreset nem találok

	return step;
}

//#4.3 Nyert valaki?
int checkLine(int line) //Megnézi, hogy a "line" sorban mind a 3 cella ugyanaz-e
{
	int x;

	int px = tiles[0][line]; //Alapul a sor első celláját veszem
	if(px == PN) //Ha az első üres akkor a többi nem is számít
		return -1;
	
	for(x = 1; x < 3; x++) //Másik 2 cella
		if(tiles[x][line] != px) //Ha nem egyezik meg az elsővel akkor nem teljes a sor
			return -1;
			
	return px; //Visszaadja, hogy a sorban kinek a karaktere van
}

int checkColumn(int column) //Megnézi, hog y a"column" oszlopban mind a 3 cella ugyanaz-e
{
	int y;
	
	int py = tiles[column][0]; //Az oszlop első celláját veszem alapul
	if(py == PN) //Ha üres akkor a többi nem számít
		return -1;
		
	for(y = 1; y < 3; y++) //Másik 2 cella
		if(tiles[column][y] != py) //Ha nem egyezik meg az elsővel akkor nem teljes a sor
			return -1;
			
	return py; //Visszaadja, hogy a sorban kinek a karaktere van
}

int checkDiagonals()
{
	int i;

	int a = tiles[0][0]; //"A": Balfent -> jobblent; első cella az alap
	int b = tiles[2][0]; //"B": Jobbfent -> ballent; első cella az alap
	for(i = 1; i < 3; i++) //2 átló másik 2 cellája
	{
		if(a != PN && a != tiles[i][i]) //Ha az "A" átló alap cellája üres akkor a többi se számít, ezt kihasználva, ha az átló egy cellája nem egyezik akkor nem nézem a többit se
			a = PN;
		if(b != PN && b != tiles[2-i][i]) //Másik átló...
			b = PN;	
	}
	
	if(a != PN) //Ha "A" átló teljes
		return a;
	if(b != PN) //Ha "B" átló teljes
		return b;
		
	return -1; //Egyik átló sem teljes
}

//Eldönti, hogy valaki nyert-e az előző 3 függvény segítségével
//-1: Nem, PN: Döntetlen, PX: X, PO: O
int won()
{
	int i, r;

	//3 sor, 3 oszlop
	for(i = 0;i<3;i++)
	{
		r = checkLine(i);
		if(r != -1)
		{
			return r;
		}
			
		r = checkColumn(i);
		if(r != -1)
		{
			return r;
		}
	}
	
	//2db átló
	r = checkDiagonals();
	if(r > -1 && r != PN) //Volt eredménye a függvénynek (>-1) és az játékos?
	{
		return r;
	}

	//Döntetlen, ha nincs olyan cella ami üres lenne
	for(i = 0;i<3;i++)
	{
		for(r = 0;r<3;r++)
		{
			if(tiles[i][r] == PN)
			{
				return -1; //Nem döntetlen
			}
		}
	}

	return PN; //Döntetlen
}

//#4.4 Maga a fő tictactoe játék
//"bot": 2. játékos számítógép?
//"playerStart": bot esetében az emberi játékos karaktere
void tictacGame(bool bot, int playerStart)
{
	int x,y;
	
	//Alaphelyzetbe állítom a terminált
	cmdResetColors();
	cmdClear();

	//Alaphelyzetbe állítom a táblát is
	resetTiles();
	
	//Kirajzolom a cellák körvonalát; (1;1) cella értelmetlen mert a többi azt is megrajzolja...
	for(x = 0;x<3;x++)
	{
		for(y = 0;y<3;y++)
		{
			drawOutline(getTilePos(x,y), gray);
		}
	}
	
	//Statikus info szöveg a helyére
	printInfoText();
	
	//Cella kurzor helye
	int cx, cy;
	cx = cy = 0;
	
	int player = PN; //Emberi játékos
	int botPlayer = PN; //Számítógép játékos
	if(bot) //Ember vs CPU esetén...
	{
		player = playerStart; //Az emberi játékos a függvénynek megadott játékos
		
		if(player != PX) //Mindig X kezd, de az emberi játékos nem X ezért a számítógép kezd
		{
			botPlayer = PX;
		
			point p = botStep(botPlayer); //Számítógép logika függvény segítségével megteszi az első lépést
			
			//A cella kurzort a lépés helyére rakom
			cx = p.x;
			cy = p.y;
			
			tiles[cx][cy] = botPlayer;
			
			cmdBgColor(COLOR_NO); //A cella kurzor piros, mert ide már nem lehet lépni
			drawPlayer(getTilePos(cx,cy), tiles[cx][cy]);
			cmdResetColors();
		}
		else //Ha az emberi játékos X akkor ő kezd
		{
			botPlayer = PO;
			
			cmdBgColor(COLOR_OK); //A cella kurzor sárga mert lehet ide lépni (nem történt még lépés)
			drawPlayer(getTilePos(cx,cy), player);
			cmdResetColors();
		}
	}
	else //Ember vs Ember esetén
	{
		player = PX; //Mindig X kezd
	
		cmdBgColor(COLOR_OK); //A cella kurzort kirajzolom sárgával
		drawPlayer(getTilePos(cx,cy), player);
		cmdResetColors();
	}
	
	bool over = false;
	int winner = PN;
	while(!over) //Játék ciklus
	{
		//Info szöveg
		cmdResetColors();
		cmdCursor(TABLE_BASE_X + TABLE_SIZE + 3, 2);
		printf("      "); //Kitöröm a "%s köre." szöveget szóközökkel
		cmdCursor(TABLE_BASE_X + TABLE_SIZE + 3, 2);
		printf("%s köre.", (player == PX ? "X" : "O"));
		
		KeyCode code = getKeyCode(); //Billentyű bemenetre várok
		
		if(code.keyType == ArrowKey) //Nyilak?
		{
			//Kurzor mozgatás
			int ocx = cx; //Mozgatás előtti hely
			int ocy = cy;
			
			switch(code.key.arrow){ //Kurzor mozgatás, úgy, hogy a kurzor ne mehessen ki a táblából
				case Left: //Bal
					cx--;
					if(cx<0) cx = 0;
					break;
				case Right: //Jobb
					cx++;
					if(cx>2) cx = 2;
					break;
				case Up: //Fel
					cy--;
					if(cy<0) cy = 0;
					break;
				case Down: //Le
					cy++;
					if(cy>2) cy = 2;
					break;
			}
			
			if(ocx != cx || ocy != cy) //A kurzor elmozdult...
			{
				cmdResetColors();
				drawPlayer(getTilePos(ocx, ocy), tiles[ocx][ocy]); //Az előző kurzor helyére kirajzolom alap színekkel a cellát
				
				point tile = getTilePos(cx, cy);
				if(tiles[cx][cy] == PN) //Az adott cellán nincs játékos?
				{
					cmdBgColor(COLOR_OK); //Lehet ide lépni
					drawPlayer(tile,  player); //Odarakom idéglenesen a játékost
				}
				else //Az adott cellán van játékos
				{
					cmdBgColor(COLOR_NO); //Nem lehet ide lépni
					drawPlayer(tile, tiles[cx][cy]); //Pirossal kirajzolom a cellát
				}
			}
		}
		else if(code.keyType == RegularKey) //Egyszerű betű
		{
			if(code.key.c == 'e' || code.key.c == 'q') //Exit || Quit
				return; //Kilép a játék függvényből
			else if(code.key.c == 10) //Enter
			{
				if(tiles[cx][cy] == PN) //Nincs játékos az adott helyen? -> Lehet rá lépni
				{
					tiles[cx][cy] = player; //Beállítom a cellát
					
					cmdBgColor(COLOR_NO); //A cella kurzort módosítom pirosra (nem lehet ide lépni) és kirajzolom
					drawPlayer(getTilePos(cx,cy), tiles[cx][cy]);
					cmdResetColors();
					
					int win = won(); //Esetleg nyert valaki?
					if(win == -1) //Még nem
					{
						if(bot) //Ha Ember vs CPU akkor a számítógép is lép
						{
							point b = botStep(botPlayer);
							
							if(b.x != cx || b.y != cy) //Ha nem a kurzor helyére lép..
							{
								cmdResetColors(); //Akkor kirajzolom a cellát az alap színnel
								drawPlayer(getTilePos(cx,cy), tiles[cx][cy]);
							}
							
							//A kurzort a lépés helyére rakom
							cx = b.x;
							cy = b.y;
							tiles[cx][cy] = botPlayer;
							
							cmdBgColor(COLOR_NO); //Kirajzolom a kurzort pirossal, mert ide már nem lehet lépni
							drawPlayer(getTilePos(cx,cy), tiles[cx][cy]);
							cmdResetColors();
							
							win = won(); //Valaki nyert?
							if(win != -1) //Van nyertes, vége...
							{
								over = true;
								winner = win;					
							}
						}
						else //Ember vs Ember
						{
							if(player == PX) //Játékos csere
								player = PO;
							else
								player = PX;
						}
					}
					else //Van nyertes, vége...
					{
						over = true;
						winner = win;
					}
				}
			}
		}
	}
	
	if(over) //Vége a játéknak?
	{
		//Kíirom a nyertest, ehhez kell 1-2 dolog
		int center = TABLE_SIZE / 2; //Tábla mérete / 2, így a szöveg a tábla közepére kerül
		
		int textX = center - 4; //A tábla közepéhez képest 4 karakterrel balra (azért négy mert a "Döntetlen" szöveg 9 karakter, ennek fele 4.5, így pont középre lehet igazítan)
		int textY = center;
		
		cmdResetColors();
		
		//Csinálok egy kis dobozt amibe a szöveg kerül, ez 1 karakterrel nagyobb mint a szöveg
		cmdBgColor(COLOR_OK);
		for(y = textY - 1; y <= textY + 1; y++) //Ez a doboz 3 sorból áll
		{
			cmdCursor(textX - 1, y);
			printf("           "); //11 szóköz, így a doboz 11 karakteres szélességén üres lesz (kitörli a tictactoe táblát)
		}
		
		//Szöveg
		cmdCursor(textX, textY);
		switch(winner){
			case PX:
				printf(" X nyert"); //Szóközökkel középre igazítom
				break;
			case PO:
				printf(" O nyert");
				break;
			case PN:
				printf("Döntetlen");
				break;
		}
		cmdResetColors();
	
		getKeyCode(); //Várok egy gombnyomást
	}
}

//#5 Egyszerű terminálos menü
int customStrlen(char* t) //A "string.h"-s strlen nem kezel ékezetes betűket
{
	 int i = 0; //String index
	 int l = 0; //String hossz

	 while(t[i] != 0) //Amég nem érek a string végére
	 {
	 	if(t[i] < 0) //Unicode esetén 2 byte van egy karakterhez, ebből az első mindig kisebb mint 0 (legalábbis a pár ékezetes karakter esetén amit használok)
	 	{
	 		i++; //+1 karakter léptetés, de a hozz nem nő
	 	}
	 	i++; //Lépek egyet a stringben
	 	l++; //És növelem a számolt hosszat is
	 }
	 
	 return l;
}

//Multifunkciós függvény ami "t" szöveget középre igazítva rajzolja egy "w" szélességű csíkban, "x" adja meg a sorban a kezdő oszlopot
//"selected": Ha true "[" és "]" kerül a szöveg elé és mögé, ezt a menü használja
//"clear": Ha true akkor a szöveg helyett csak szóközök kerülnek kiírásra így az előzőleg kiírt szöveg "törlődik"
void printCenter(char* t, int x, int w, int y, bool selected, bool clear)
{
	int len = customStrlen(t); //Szöveg hossza
	
	int textX = x + (w / 2) - (len / 2); //"x"-el eltolom a szöveget és középre igazítom
	
	if(clear) //Törlés esetén
	{
		cmdCursor(textX - 1, y); //-1, hogy a "[" is törlődjön
		int i;
		for(i = 0; i < len + 2; i++) //Hossz + 2 karakter ("[" és "]") törlése
			printf(" ");
	
		return;
	}
	
	//Kijelölés
	cmdCursor(textX - 1, y);
	printf("%s", (selected ? "[" : " ")); //Azért kell szóköz, mert ha előzőleg ki volt jelölve akkor a kijelölő karaktert törölni kell
		
	cmdCursor(textX+len,y);
	printf("%s", (selected ? "]" : " "));
	
	//Szöveg
	cmdCursor(textX, y);
	printf("%s", t);
}

//Függvény mely egy 3 elemű menüben a kiválasztott elemet adja vissza (-1 ha kilépés)
//"posY": Megadja a menü első sorát
int selectMenu(char a[3][16], int posY)
{
	int selected = 0; //Melyik string van kiválasztva?
	
	//A menü X helye és szélessége, eredetileg paraméterek voltak, de mivel a 2 menü esetén megegyezik, így változóként logikusabb
	int posX = TABLE_BASE_X + 2;
	int posW = 22;
	
	//Menü alaphelyzetbe
	int x;
	for(x = 0; x < 3; x++)
	{
		printCenter(&a[x][0], posX, posW, TABLE_BASE_Y + posY + x, (x == selected), false); //A sor helyét a "posY" és a sor indexe adja meg (posY + x)
	}
	
	while(true)
	{
		KeyCode code = getKeyCode(); //Gombnyomást várok
		if(code.keyType == ArrowKey)
		{
			int old = selected; //Eltárolom, hogy mi volt kiválasztva
		
			if(code.key.arrow == Up) //Felfelé mozgatás
			{
				selected--;
				if(selected < 0) //Ha kimegyek a menüből akkor körbefordul
					selected = 2;
			}
			else if(code.key.arrow == Down)
			{
				selected++;
				if(selected > 2)
					selected = 0;
			}
			
			printCenter(&a[old][0], posX, posW, TABLE_BASE_Y + posY + old, false, false); //Az előző menü sor kijelölését törlöm
			printCenter(&a[selected][0], posX, posW, TABLE_BASE_Y + posY + selected, true, false); //Kirajzolom az új sor kijelölését
		}
		else if(code.keyType == RegularKey) //Egyszerű betű
		{
			if(code.key.c == 'e' || code.key.c == 'q') //Exit || Quit
				return -1; //-1 = kilépés
			else if(code.key.c == 10) //Enter
			{
				break; //Visszatérés a választott értékkel
			}
		}
	}
	
	//Kiürítem a 3 sort az új menüknek
	for(x = 0;x<3;x++)
	{
		printCenter(&a[x][0], posX, posW, TABLE_BASE_Y + posY + x, false, true);
	}
	
	return selected;
}

//A 2 menü lehetőségei
char mainMenu[3][16] =
{
	"Ember vs ember",
	"Ember vs CPU",
	"Kilépés"
};

char botMenu[3][16] =
{
	"X",
	"O",
	"Vissza"
};

//A játék menüje
bool tictacMenu()
{
	int x,y;
	
	cmdResetColors();

	//Menu "körvonala"
	int tableHeight = 12;
	int tableWidth = 27;
	
	//TicTacToe felirat
	printCenter("TicTacToe", TABLE_BASE_X, tableWidth - 1, TABLE_BASE_Y + 2, false, false);
	
	cmdBgColor(gray); //Körvonal színe
	
	//A tictactoe tábla bal felső sarkából kiindulva rajzolom ki a menü ablakot is
	for(x = TABLE_BASE_X; x<TABLE_BASE_X+tableWidth;x++) //Vonalak
	{
		cmdCursor(x, TABLE_BASE_Y); //Keret teteje
		printf(" ");
		
		cmdCursor(x, TABLE_BASE_Y + tableHeight - 1); //Keret alja
		printf(" ");
	}
	
	for(y = TABLE_BASE_Y; y<TABLE_BASE_Y+tableHeight;y++)
	{
		cmdCursor(TABLE_BASE_X, y);
		printf(" ");
		
		cmdCursor(TABLE_BASE_X+tableWidth-1,y);
		printf(" ");
	}
	
	cmdResetColors();
	
	//Menü
	int sel;
	int menuMode = 0; //0: Főmenü, 1: X/O a játékos (ember vs cpu)
	while(true)
	{
		switch(menuMode)
		{
			case 0: //Főmenü
			{			
				sel = selectMenu(mainMenu, 4);
				switch(sel)
				{
					case 0: //Ember vs Ember
						tictacGame(false, PN); //Nincs számítógépes játékos, 2. paraméter pedig ezért teljesen mindegy
						return true;
					case 1: //Bot vs ember
						menuMode = 1; //Kell a másik menü
						break;
					case 2: //Kilépés
					default: //(-1 azaz Q vagy E esetén)
						return false; //Kilépés
				}
			}
			break;
			case 1: //Ember vs CPU -> ember karaktere?
			{
				printCenter("Játékos?", TABLE_BASE_X, tableWidth, TABLE_BASE_Y + 4, false, false); //Almenü "fejléce"
			
				sel = selectMenu(botMenu, 6);
				switch(sel)
				{
					case 0: //X
						tictacGame(true, PX);
						return true;
					case 1: //O
						tictacGame(true, PO);
						return true;
					case 2: //Vissza
						menuMode = 0;
						break;
					default: //Kilépés
						return false;
				}

				printCenter("        ", TABLE_BASE_X, tableWidth, TABLE_BASE_Y + 4, false, false); //Törlöm az almenü fejlécét szóközökkel
			}
			break;
		}
	}
}

//#6 Belépési pont
int main()
{
	//Beállítom a terminált
	cmdCursorBlink(0);
	cmdResetColors(); //Színek alaphelyzetbe..
	cmdClear(); //..terminál törlése
	
	srand(time(NULL)); //Random seedelés

	//Játék
	while(tictacMenu()) //Amég nem dönt úgy a játékos, hogy kilép
	{
		cmdClear(); //Törlöm a terminált és mehet a menü újra
	}

	//Kilépés:
	cmdCursorBlink(1); //Kurzor villogás vissza
	cmdResetColors(); //Színek alaphelyzetbe
	cmdClear(); //Terminál törlése
	cmdCursor(0,0); //Kurzor bal felső sarokba
	
	return 0;
}
