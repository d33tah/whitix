#include <i386/ioports.h>
#include <typedefs.h>
#include <print.h>

BYTE* screen=(BYTE*)0xC00B8000;
int curX, curY;

void EarlyConsoleSetPos(int x, int y);

void EarlyConsoleNewLine()
{
	curY++;
	curX=0;

	if (curY < 25)
		EarlyConsoleSetPos(curX, curY);
	else{
//		EarlyConsoleClearScreen();
//		curX=curY=0;
	}
}

void EarlyConsolePrint(char* string, int length)
{
	int i;

	for (i=0; i<length; i++)
	{
		switch (string[i])
		{
			case '\n':
				EarlyConsoleNewLine();
				break;

			default:
				screen[(curY*(80*2))+(curX*2)]=string[i];
				screen[(curY*(80*2))+(curX*2)+1]=7;
				curX++;
		}
		
		if (curX >= 80)
			EarlyConsoleNewLine();
	}
}

void EarlyConsoleSetPos(int x, int y)
{
	WORD position=(y*80)+x;

	curX=x;
	curY=y;

	outb(0x3D4,14);
	outb(0x3D5,(BYTE)(position >> 8));
	outb(0x3D4,15);
	outb(0x3D5,(BYTE)(position & 0xFF));
}

void EarlyConsoleClearScreen()
{
	int i;

	for (i=0; i<(80*25*2); i+=2)
	{
		screen[i]=' ';
		screen[i+1]=7;
	}

	EarlyConsoleSetPos(0, 0);
}

void EarlyConsoleInit()
{
	EarlyConsoleClearScreen();
	KeSetOutput(EarlyConsolePrint);
}
