#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#define POOLSIZE  (256 * 1024)  
#define SYMTABSIZE (1024*8)     
#define MAXNAMESIZE 32          
#define RETURNFLAG DBL_MAX      
using namespace std;

char* src, * old_src;           

typedef struct symStruct
{
  /* data */
  int type;
  char name[MAXNAMESIZE];
  double value;
  union
  {
    char* funcp;
    struct symStruct* list;
  }pointer;
  int levelNum;
}symbol;

symbol symtab[SYMTABSIZE];
int symPointer = 0;
int currentlevel = 0;
int token;
union tokenValue
{
  symbol* ptr;
  double val;
}token_val;

enum {
  debug, run
};

int compileState = run;

double return_val = 0;


enum {
  Num = 128, Char, Str, Array, Func, Else, If, Return, While, Print, Puts, Read, Assign, OR, AND, 
  Equal, Sym, FuncSym, ArraySym, Void, Nequal, LessEqual, GreatEqual, Inc, Dec
};



double function();
double statement();
int boolOR();
int boolAND();
int boolexp();
double expression();
double factor();
double term();
void match(int tk);
void next();

//词法分析
void next()
{
  char* last_pos;
  while (token = *src)
  {
    ++src;
    if (token == '\n')
    {
      old_src = src;
    }
    else if (token == ' ' || token == '\t')
    {

    }
    else if (token == '#')
    {
      while(*src != 0 && *src !='n')
      {
        src++;
      }
    }
    else if (token == '*' || token == '/' || token == ';' || token == ',' ||
    token == '(' ||token == ')' || token == '{', token == '}' || token == '[' || token == ']')
    {
      return;
    }
    else if (token >= '0' && token <= '9')
    {
      token_val.val = (double)token - '0';
      while (*src >= '0' && *src <= '9')
      {
        token_val.val = token_val.val * 10 + *src++ -'0';
      }
      if (*src = '.')
      {
        src++;
        int countDig = 1;
        while (*src >= '0' && *src <= '9')
        {
          token_val.val = token_val.val + ((double)(*src++) - '0')/(10.0 * countDig++);
        }
      }
      token = Num;
      return;
    }
    else if (token == '"')
    {
      last_pos = src;
      int numCount = 0;
      while (*src != 0 && *src != token)
      {
        src++;
        numCount++;
      }
      if (*src)
      {
        *src = 0;
        token_val.ptr = (symbol*)malloc(sizeof(char) * numCount + 8);
        strcpy((char*)token_val.ptr, last_pos);
        *src = token;
        src++;
      }
      token = Str;
      return;
    }
    else if (token == '\'')
    {
      token_val.val = *src++;
      token = Char;
      src++;
      return;
    }
    else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_'))
    {
      last_pos = src - 1;
      char nameBuffer[MAXNAMESIZE];
      nameBuffer[0] = token;
      while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_'))
      {
        nameBuffer[src-last_pos] = *src;
        src++;
      }
      nameBuffer[src - last_pos] = 0;
      int i;
      for (i = symPointer - 1; i>= 0; --i)
      {
        if (strcmp(nameBuffer, symtab[i].name)==0)
        {
          if (symtab[i].type == Num || symtab[i].type == Char)
          {
            token_val.ptr = &symtab[i];
            token = Sym;
          }
          else if (symtab[i].type == FuncSym)
          {
            token_val.ptr = &symtab[i];
            token = symtab[i].type;
          }
          else if (symtab[i].type == ArraySym)
          {
            token_val.ptr = &symtab[i];
            token = symtab[i].type;
          }
          else 
          {
            if (symtab[i].type == Void)
            {
              token = Sym;
              token_val.ptr = &symtab[i];
            }
            else token = symtab[i].type;
          }
          return;
        }
      }
      strcpy(symtab[symPointer].name, nameBuffer);
      symtab[symPointer].levelNum = currentlevel;
      symtab[symPointer].type = Void;
      token_val.ptr = &symtab[symPointer];
      symPointer++;
      token = Sym;
      return;
    }
    else if (token == '=')
    {
      if (*src == '=')
      {
        src++;
        token = Equal;
      }
      return;
    }
    else if (token == '+')
    {
      if (*src == '+')
      {
        src++;
        token = Inc;
      }
      return;
    }
    else if (token == '-')
    {
      if (*src == '-')
      {
        src++;
        token = Dec;
      }
      return;
    }
    else if (token == '!')
    {
      if (*src == '!')
      {
        src++;
        token = Nequal;
      }
      return;
    }
    else if (token == '<')
    {
      if (*src == '<')
      {
        src++;
        token = LessEqual;
      }
      return;
    }
    else if (token == '>')
    {
      if (*src == '>')
      {
        src++;
        token = GreatEqual;
      }
      return;
    }
    else if (token == '|')
    {
      if (*src == '|')
      {
        src++;
        token = OR;
      }
      return;
    }
    else if (token ==  '&')
    {
      if (*src = '&')
      {
        src++;
        token = AND;
      }
      return;
    }
    else 
    {
      printf("unexpected token:%d\n", token);
    }
  }
}

void match(int tk)
{
  if (token == tk)
  {
    next();
  }
  else
  {
    exit(-1);
  }
}
//语法分析
double expression()
{
  double temp = term();
  while(token == '+' || token == '-')
  {
    if (token == '+')
    {
      match('+');
      temp += term();
    }
    else
    {
      match('-');
      temp -= term();
    }
  }
  return temp;
}

double factor()
{
  double temp = 0;
  if (token == '(')
  {
    match('(');
    temp = expression();
    match(')');
  }
  else if (token == Num || token == Char)
  {
    temp = token_val.val;
    match(Num);
  }
  else if (token == Sym)
  {
    temp = token_val.ptr->value;
    match(Sym);
  }
  else if (token == FuncSym)
  {
    return function();
  }
  else if (token == ArraySym)
  {
    symbol* ptr = token_val.ptr;
    match(ArraySym);
    match('[');
    int index = (int)expression();
    if (index >= 0 && index < ptr->value)
    {
      temp = ptr->pointer.list[index].value;
    }
    match(']');
  }
  return temp;
}

double term()
{
  double temp = factor();
  while (token == '*' || token == '/')
  {
    if (token == '*')
    {
      match('*');
      temp *= factor();
    }
    else
    {
      match('/');
      temp /= factor();
    }
  }
  return temp;
}

int boolexp()
{
  if (token == '(')
  {
    match('(');
    int reseult = boolOR();
    match(')');
    return reseult;
  }
  else if (token == '!')
  {
    match('!');
    return boolexp();
  }
  double temp = expression();
  if (token == '>')
  {
    match('>');
    return temp > expression();
  }
  else if (token == '<')
  {
    match('<');
    return temp < expression();
  }
  else if (token == GreatEqual)
  {
    match(GreatEqual);
    return temp >= expression();
  }
  else if (token == LessEqual)
  {
    match(LessEqual);
    return temp <= expression();
  }
  else if (token == Equal)
  {
    match(Equal);
    return temp ==expression();
  }
  return 0;
}

void skipBoolExpr() 
{
    int count = 0;
    while (token && !(token == ')' && count == 0)) 
    {
        if (token == '(') count++;
        if (token == ')') count--;
        token = *src++;
    }
}

int boolAND()
{
  int val = boolexp();
  while(token == AND)
  {
    match(AND);
    if (val == 0) 
    {
      skipBoolExpr();
      return 0;
    }
    val = val & boolexp();
    if (val == 0) return 0;
  }
  return val;
}

int boolOR()
{
  int val = boolAND();
  while(token == OR)
  {
    match(OR);
    if (val == 1) 
    {
      skipBoolExpr();
      return 1;
    }
    val = val | boolAND();
    if (val == 1) return 1;
  }
  return val;
}

void skipStatements()
{
  if (token == '{')
  {
    token = *src++;
  }
  int count = 0;
  while (token && !(token == '}' && count == 0))
  {
    if (token == '}') count++;
    if (token == '{') count--;
    token = *src++;
  }
  match('}');
}

double statement()
{
  if (token == '{')
  {
    match('{');
    while (token != '}')
    {
      if (RETURNFLAG == statement()) return RETURNFLAG;
    }
    match('}');
  }
  else if (token == If)
  {
    match(If);
    match('(');
    int boolresult = boolOR();
    match(')');
    if (boolresult)
    {
      if (RETURNFLAG == statement())
      {
        return RETURNFLAG;
      }
    }
    else skipStatements();
    if (token == Else)
    {
      match(Else);
      if (!boolresult)
      {
        if (RETURNFLAG == statement())
        {
          return RETURNFLAG;
        }
      }
      else skipStatements();
    }
  }
  else if (token == While)
  {
    match(While);
    char* whileStartPos = src;
    char* whileStartOldPos = old_src;
    int boolresulr;
    do{
      src = whileStartPos;
      old_src = whileStartOldPos;
      token = '(';
      match('(');
      boolresulr = boolOR();
      match(')');
      if (boolresulr)
      {
        if (RETURNFLAG == statement())
        {
          return RETURNFLAG;
        }
      }
      else skipStatements();
    }while (boolresulr);
  }
  else if (token == Sym || token == ArraySym)
  {
    symbol* s = token_val.ptr;
    int tktype = token;
    int index;
    match(tktype);
    if (tktype == ArraySym)
    {
      match('[');
      index = expression();
      match(']');
      match('=');
      if (index >= 0 && index < s->value)
      {
        s->pointer.list[index].value = expression();
      }
    }
    else
    {
      match('=');
      if (token ==Str)
      {
        s->pointer.funcp = (char*)token_val.ptr;
        s->type = Str;
        match(Str);
      }
      else if (token == Char)
      {
        s->value = token_val.val;
        s->type = Char;
        match(Char);
      }
      else
      {
        s->value = expression();
        s->type =Num;
      }
    }
    match(';');
  }
  else if (token == Func)
  {
    match(Func);
    symbol* s= token_val.ptr;
    s->type = FuncSym;
    match(Sym);
    s->pointer.funcp = src;
    s->value = token;
    skipStatements();
    match(';');
  }
  else if (token == Array)
  {
    match('Array');
    symbol* s= token_val.ptr;
    match(Sym);
    match('(');
    int length = (int)expression();
    match(')');
    s->pointer.list = (struct symStruct*)malloc(sizeof(struct symStruct) * length + 1);
    for (int i = 0; i < length; ++i)
    {
      s->pointer.list[i].type = Num;
    }
    s->value = length;
    s->type = ArraySym;
    match(';');
  }
  else if (token == Return)
  {
    match(Return);
    match('(');
    return_val = expression();
    match(')');
    match(';');
    return RETURNFLAG;
  }
  else if (token == Print || token == Read || token == Puts)
  {
    int func = token;
    double temp;
    match(func);
    match('(');
    switch (func)
    {
    case Print:
      temp = expression();
      printf("%lf\n", temp);
      break;
    case Puts:
      printf("%s\n", (char*)token_val.ptr);
      break;
    case Read:
      scanf("%lf", &token_val.ptr->value);
      token_val.ptr->type = Num;
      match(Sym);
    }
    match(')');
    match(';');
  }
  return 0;
}

double function()
{
  currentlevel++;
  return_val = 0;
  symbol* s = token_val.ptr;
  match(FuncSym);
  match('(');
  while (token != ')')
  {
    symtab[symPointer] = *token_val.ptr;
    strcpy(symtab[symPointer].name, token_val.ptr->name);
    symtab[symPointer].levelNum = currentlevel;
    symPointer++;
    match(Sym);
    if (token == ',') match(',');
  }
  match(')');
  char* startPos = src;
  char* startOldPos = old_src;
  int startToken = token;
  old_src = src = s->pointer.funcp;
  token = (int)s->value;
  statement();
  src = startPos;
  old_src = startOldPos;
  token = startToken;
  while (symtab[symPointer - 1].levelNum == currentlevel)
  {
    symPointer--;
  }
  currentlevel--;
  return return_val;
}

int main(int argc, char** argv)
{
  int i, fd;
  src = "array func else if return while print puts read";
  for (i = Array; i <= Read; ++i)
  {
    next();
    symtab[symPointer - 1].type = i;
  }
  if (!(src = old_src = (char*)malloc(POOLSIZE)))
  {
    printf("could not malloc(%d) for source area\n", POOLSIZE);
    return -1;
  }
  ++argv;
  --argc;
  if (argc > 0)
  {
    if (**argv == '-' && (*argv)[i] == 'd')
    {
      compileState = debug;
      ++argv;
      --argc;
    }
    else compileState = run;
  }
  if (argc < 1)
  {
    printf("usage: tryc [-d] file ...\n");
    return -1;
  }
  if ((fd = open(*argv, 0)) < 0)
  {
    printf("could not open(%s)\n", *argv);
    return -1;
  }
  if ((i = read(fd, src, POOLSIZE - 1)) <= 0)
  {
    printf("read() returned %d\n", i);
    return -1;
  }
  src[i] = 0;
  close(fd);
  next();
  while (token != 0)
  {
    statement();
  }
  return 0;
}
/*这是一个例子
# Fibonacci sequence
func fun{ 
	if(x <= 2){  
		return(1);  
	}
	y = 0;
	x = x - 1;
	y = fun(x);
	x = x - 1;
	return(y + fun(x));
};

# save the Fibonacci sequence of 1 to 15 in an array 
array arr(15);	
x = 1;
while( x <= 15 ){
	arr[x - 1] = fun(x);
	x = x + 1;
}

puts("Fibonacci sequence:");
# print the Fibonacci sequence of 1 to 15
i = 0;
while(i < 15){			
	print(arr[i]);
	i=i+1;
}
*/