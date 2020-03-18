#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "table.h"

#define HASHTABLE_SIZE 100

void preprocess(struct table *t, FILE *inputStream, const char outputFile[]);

char symbols[256][256];
int symbolsCount;

char directories[256][256];
int directoryCount;

int main(int argc, char **argv)
{
	struct table *t = createTable(HASHTABLE_SIZE);
	int i = 1;
	char symbol[256] = "", mapping[256] = "";
	char *p, *q;
	FILE *inputStream;
	char outputFile[256] = "";
	char directoriesTmp[256][256];
	int directoryCountTmp = 0;
	int equalPosition;

	//parse define arguments
	while (i < argc) {
		if (strcmp(argv[i], "-D") == 0) {
			//with space between -D and symbol
			p = argv[i + 1];
			i += 2;
		} else if (memcmp(argv[i], "-D", 2) == 0) {
			//without space between -D and symbol
			p = argv[i] + 2;
			i++;
		} else
			break;

		q = strchr(p, '=');

		if (q == NULL) {
			strcpy(symbol, p);
			strcpy(mapping, "");
		} else {
			equalPosition = strchr(p, '=') - p;
			strcpy(symbol, p);
			symbol[equalPosition] = '\0';
			strcpy(mapping, p + equalPosition + 1);
		}

		insert(t, symbol, mapping);
		strcpy(symbols[symbolsCount], symbol);
		symbolsCount++;
	}

	//add current directory
	strcpy(directories[directoryCount], ".");
	directoryCount++;
	while (i < argc && strcmp(argv[i], "-I") == 0) {
		strcpy(directoriesTmp[directoryCountTmp], argv[i + 1]);
		directoryCountTmp++;
		i += 2;
	}

	//check for invalid arguments
	if (i < argc && argv[i][0] == '-' && strcmp(argv[i], "-o") != 0) {
		destroyTable(t);
		return -1;
	}

	if (i < argc && strcmp(argv[i], "-o") != 0) {
		//add the directory of the input file
		strcpy(directories[directoryCount], argv[i]);
		p = strrchr(directories[directoryCount], '/');
		if (p != NULL) {
			*p = '\0';
			directoryCount++;
		}

		//open file
		inputStream = fopen(argv[i], "r");
		i++;
		if (inputStream == NULL) {
			destroyTable(t);
			return -1;
		}

		//parse output file argument (without -o)
		if (i < argc &&  strcmp(argv[i], "-o") != 0) {
			strcpy(outputFile, argv[i]);
			i++;
			if (i < argc &&  strcmp(argv[i], "-o") != 0) {
				//multiple output files means invalid arguments
				destroyTable(t);
				return -1;
			}
		}
	} else
		inputStream = stdin;

	//parse output file argument (with -o)
	if (i < argc && strcmp(argv[i], "-o") == 0) {
		strcpy(outputFile, argv[i + 1]);
		i += 2;
	}

	//add directories specified with -I arguments - lowest priotiry
	for (i = 0; i < directoryCountTmp; i++) {
		strcpy(directories[directoryCount], directoriesTmp[i]);
		directoryCount++;
	}

	preprocess(t, inputStream, outputFile);

	destroyTable(t);

	return 0;
}

void readStreamContents(FILE *inputStream, char **buffer)
{
	long fsize;
	char *p;

	if (inputStream == stdin)
		fsize = 1024;
	else {
		fseek(inputStream, 0, SEEK_END);
		fsize = ftell(inputStream);
		if (fsize < 0) {
			*buffer = NULL;
			return;
		}
		fseek(inputStream, 0, SEEK_SET);
	}

	p = malloc(10 * fsize + 1);
	if (p == NULL)
		return;

	fsize = fread(p, 1, fsize, inputStream);
	fclose(inputStream);

	p[fsize] = '\0';
	*buffer = p;
}

char *findDefine(char *input)
{
	char *p = input, *q;
	int quoteCount;

	p = strstr(input, "#define");
	while (1) {
		quoteCount = 0;
		if (p == NULL)
			return NULL;

		q = input;
		while (p != q) {
			if (*q == '\"')
				quoteCount++;

			q++;
		}

		if (quoteCount % 2 == 0)
			return p;
		p = strstr(p + 1, "#define");
	}
}

char *findIfTrueDirective(char *input)
{
	char *p = input;

	while (1) {
		if (p == NULL)
			return NULL;

		if (isdigit(p[4]))
			return p;

		p = strstr(p + 1, "#if ");
	}
}

void findSymbolToReplace(char *buffer, char **symbolFound, int *length)
{
	char separators[32];
	int i;
	char *p;

	strcpy(separators, " \t\r\n[]{}<>=+-*/%!&|^.,:;()\\");

	for (i = 0; i < symbolsCount; i++) {
		p = buffer;

		while ((p = strstr(p, symbols[i])) != NULL) {
			*symbolFound = p;
			*length = strlen(symbols[i]);
			if (strchr(separators, *(p - 1)) != NULL
				&& strchr(separators, p[*length]) != NULL)

				return;
			p++;
		}
	}

	*symbolFound = NULL;
	*length = 0;
}

void getNextToken(char *output, const char *input, const char *separators)
{
	const char *p, *q;

	p = input;
	while (*p != '\0' && strchr(separators, *p))
		p++;

	q = p;
	while (*p != '\0' && strchr(separators, *q) == NULL)
		q++;

	memcpy(output, p, q - p);
	output[q - p] = '\0';
}

void addDefine(char *input, struct table *t)
{
	const char *p;
	char separators[16];
	char token[256];
	char symbol[256];
	char mapping[256] = "";

	strcpy(separators, " \t\r\n");
	getNextToken(symbol, input + strlen("#define"), separators);

	p = input + strlen("#define") + strlen(symbol) + 1;
	while (1) {
		strcpy(separators, " \t");
		while (*p != '\0' && strchr(separators, *p))
			p++;
		strcpy(separators, "\r\n");
		if (*p != '\n')
			getNextToken(token, p, separators);
		else {
			strcpy(mapping, "");
			break;
		}

		if (strchr(token, '\\') == NULL) {
			strcat(mapping, token);
			p = p + strlen(token);
			break;
		}

		token[strlen(token) - 1] = '\0';
		strcat(mapping, token);
		p = p + strlen(token) + 2;
	}

	insert(t, symbol, mapping);
	strcpy(symbols[symbolsCount], symbol);
	symbolsCount++;

	strcpy(separators, " \t");
	while (*p != '\0' && strchr(separators, *p))
		p++;
	if (*p == '\r')
		p++;
	if (*p == '\n')
		p++;

	memcpy(input, p, strlen(p));
	input[strlen(p)] = '\0';

}

void replaceSymbol(char *symbolFound, int length, struct table *t)
{
	char tmp[256];
	char symbol[256];
	char *p;

	strcpy(tmp, symbolFound + length);
	strcpy(symbol, symbolFound);
	symbol[length] = '\0';
	p = lookup(t, symbol);
	strcpy(symbolFound, p);
	strcat(symbolFound, tmp);
}

void removeDefine(char *input, struct table *t)
{
	char *p, *q;
	char separators[16];
	char *symbolStart;
	char *symbolEnd;
	char symbol[256];
	int i, indexToRemove;

	strcpy(separators, " \t\r\n");

	p = input;

	symbolStart = p + strlen("#undef");
	while (strchr(separators, *symbolStart))
		symbolStart++;

	symbolEnd = symbolStart;
	while (strchr(separators, *symbolEnd) == NULL)
		symbolEnd++;

	strcpy(symbol, symbolStart);
	symbol[symbolEnd - symbolStart] = '\0';

	q = symbolEnd;
	strcpy(separators, " \t");
	while (*q != '\0' && strchr(separators, *q))
		q++;
	if (*q == '\r')
		q++;
	if (*q == '\n')
		q++;

	memcpy(p, q, strlen(q));
	p[strlen(q)] = '\0';

	removeKey(t, symbol);
	indexToRemove = -1;
	for (i = 0; i < symbolsCount; i++)
		if (strcmp(symbols[i], symbol) == 0) {
			indexToRemove = i;
			break;
		}

	if (indexToRemove != -1) {
		for (i = indexToRemove; i < symbolsCount - 1; i++)
			strcpy(symbols[i], symbols[i + 1]);
		symbolsCount--;
	}
}

void removeIfTrueDirective(char *input)
{
	char *p, *q, *elsePtr, *elifPtr;

	p = input + strlen("#if 1");
	if (*p == '\r')
		p++;
	if (*p == '\n')
		p++;
	memcpy(input, p, strlen(p));
	input[strlen(p)] = '\0';

	elsePtr = strstr(input, "#else");
	elifPtr = strstr(input, "#elif");
	if (elifPtr != NULL && elifPtr < elsePtr)
		elsePtr = elifPtr;
	p = strstr(input, "#endif");

	q = p + strlen("#endif");
	if (*q == '\r')
		q++;
	if (*q == '\n')
		q++;

	if (elsePtr != NULL && elsePtr < p)
		p = elsePtr;

	memcpy(p, q, strlen(q));
	p[strlen(q)] = '\0';
}

void removeIfFalseDirective(char *input)
{
	char *p, *q, *elsePtr, *elifPtr;

	q = strstr(input, "#endif") + strlen("#endif");
	elsePtr = strstr(input, "#else");
	if (*q == '\r')
		q++;
	if (*q == '\n')
		q++;

	if (elsePtr > q) {
		memcpy(input, q, strlen(q));
		input[strlen(q)] = '\0';
	} else {
		q = elsePtr + strlen("#else");
		elifPtr = strstr(input, "#elif");
		if (elifPtr != NULL && (elifPtr < elsePtr || elsePtr == NULL)) {
			elifPtr[2] = '#';
			memcpy(input, elifPtr + 2, strlen(elifPtr + 2));
			input[strlen(elifPtr + 2)] = '\0';
		} else {
			if (elsePtr != NULL)
				q = elsePtr + strlen("#else");
			else
				q = strstr(input, "#endif");
			memcpy(input, q, strlen(q));
			input[strlen(q)] = '\0';

			p = strstr(input, "#endif");
			q = p + strlen("#endif");
			memcpy(p, q, strlen(q));
			p[strlen(q)] = '\0';
		}
	}
}

void removeIfDefDirective(char *input)
{
	char *p, *q;
	char separators[16], key[256];
	int i, symbolExists = 0;

	strcpy(separators, " \t\r\n");
	p = input + strlen("#ifdef");
	while (*p != '\0' && strchr(separators, *p))
		p++;
	getNextToken(key, p, separators);
	for (i = 0; i < symbolsCount; i++)
		if (strcmp(key, symbols[i]) == 0) {
			symbolExists = 1;
			break;
		}

	if (symbolExists) {
		p += strlen(key) + 1;
		memcpy(input, p, strlen(p));
		input[strlen(p)] = '\0';

		p = strstr(input, "#endif");
		q = p + strlen("#endif") + 1;
		memcpy(p, q, strlen(q));
		p[strlen(q)] = '\0';
	} else {
		q = strstr(input, "#endif") + strlen("#endif") + 1;
		memcpy(input, q, strlen(q));
		input[strlen(q)] = '\0';
	}
}

void removeIfNdefDirective(char *input)
{
	char *p, *q;
	char separators[16], key[256];
	int i, symbolExists = 0;

	strcpy(separators, " \t\r\n");
	p = input + strlen("#ifndef");
	while (*p != '\0' && strchr(separators, *p))
		p++;
	getNextToken(key, p, separators);
	for (i = 0; i < symbolsCount; i++)
		if (strcmp(key, symbols[i]) == 0) {
			symbolExists = 1;
			break;
		}

	if (!symbolExists) {
		p += strlen(key) + 1;
		memcpy(input, p, strlen(p));
		input[strlen(p)] = '\0';

		p = strstr(input, "#endif");
		q = p + strlen("#endif") + 1;
		memcpy(p, q, strlen(q));
		p[strlen(q)] = '\0';
	} else {
		q = strstr(input, "#endif") + strlen("#endif") + 1;
		memcpy(input, q, strlen(q));
		input[strlen(q)] = '\0';
	}
}

void findFilePath(FILE **file, const char *fileName)
{
	char filePath[256];
	int i;

	for (i = 0; i < directoryCount; i++) {
		strcpy(filePath, directories[i]);
		strcat(filePath, "/");
		strcat(filePath, fileName);
		*file = fopen(filePath, "r");
		if (*file != NULL)
			return;
	}
}

void removeIncludeDirective(char *input)
{
	FILE *file;
	char fileName[256], *fileContents;
	char *p = strchr(input, '\"');
	char *q = strchr(p + 1, '\"');
	char tmp[256];

	memset(fileName, 0, sizeof(fileName));
	strncpy(fileName, p + 1, q - p - 1);
	findFilePath(&file, fileName);
	if (file == NULL)
		exit(-1);
	readStreamContents(file, &fileContents);
	if (fileContents == NULL)
		exit(-1);

	q++;
	if (*q == '\r')
		q++;
	if (*q == '\n')
		q++;
	strcpy(tmp, q);
	strcpy(input, fileContents);
	strcat(input, tmp);

	free(fileContents);
}

void removeComment(char *input, const char *endingString)
{
	char *p = strstr(input, endingString);

	p += strlen(endingString);
	memcpy(input, p, strlen(p));
	input[strlen(p)] = '\0';
}

void preprocess(struct table *t, FILE *inputStream, const char outputFile[])
{
	char *min, *defineToAdd, *symbolToReplace, *defineToRemove;
	char *ifTrueDirectiveToRemove, *ifFalseDirectiveToRemove;
	char *ifDefDirectiveToRemove, *ifNdefDirectiveToRemove;
	char *includeDirectiveToRemove, *singleLineCommentToRemove;
	char *multilineCommentToRemove, *buffer = NULL;
	FILE *outputStream;
	int length;

	readStreamContents(inputStream, &buffer);

	while (1) {
		defineToAdd = findDefine(buffer);
		findSymbolToReplace(buffer, &symbolToReplace, &length);
		defineToRemove = strstr(buffer, "#undef");
		ifFalseDirectiveToRemove = strstr(buffer, "#if 0");
		ifTrueDirectiveToRemove = findIfTrueDirective(buffer);
		ifDefDirectiveToRemove = strstr(buffer, "#ifdef");
		ifNdefDirectiveToRemove = strstr(buffer, "#ifndef");
		includeDirectiveToRemove = strstr(buffer, "#include");
		singleLineCommentToRemove = strstr(buffer, "//");
		multilineCommentToRemove = strstr(buffer, "/*");

		min = NULL;
		if (defineToAdd != NULL
				&& (defineToAdd < min
				|| min == NULL))
			min = defineToAdd;
		if (symbolToReplace != NULL
				&& (symbolToReplace < min || min == NULL))
			min = symbolToReplace;
		if (defineToRemove != NULL
				&& (defineToRemove < min || min == NULL))
			min = defineToRemove;
		if (ifFalseDirectiveToRemove != NULL
				&& (ifFalseDirectiveToRemove < min
				|| min == NULL))
			min = ifFalseDirectiveToRemove;
		if (ifTrueDirectiveToRemove != NULL
				&& (ifTrueDirectiveToRemove < min
				|| min == NULL))
			min = ifTrueDirectiveToRemove;
		if (ifDefDirectiveToRemove != NULL
				&& (ifDefDirectiveToRemove < min
				|| min == NULL))
			min = ifDefDirectiveToRemove;
		if (ifNdefDirectiveToRemove != NULL
				&& (ifNdefDirectiveToRemove < min
				|| min == NULL))
			min = ifNdefDirectiveToRemove;
		if (includeDirectiveToRemove != NULL
				&& (includeDirectiveToRemove < min
				|| min == NULL))
			min = includeDirectiveToRemove;
		if (singleLineCommentToRemove != NULL
				&& (singleLineCommentToRemove < min
				|| min == NULL))
			min = singleLineCommentToRemove;
		if (multilineCommentToRemove != NULL
				&& (multilineCommentToRemove < min
				|| min == NULL))
			min = multilineCommentToRemove;

		if (min == NULL)
			break;

		if (min == defineToAdd)
			addDefine(defineToAdd, t);
		else if (min == symbolToReplace)
			replaceSymbol(symbolToReplace, length, t);
		else if (min == defineToRemove)
			removeDefine(defineToRemove, t);
		else if (min == ifFalseDirectiveToRemove)
			removeIfFalseDirective(ifFalseDirectiveToRemove);
		else if (min == ifTrueDirectiveToRemove)
			removeIfTrueDirective(ifTrueDirectiveToRemove);
		else if (min == ifDefDirectiveToRemove)
			removeIfDefDirective(ifDefDirectiveToRemove);
		else if (min == ifNdefDirectiveToRemove)
			removeIfNdefDirective(ifNdefDirectiveToRemove);
		else if (min == includeDirectiveToRemove)
			removeIncludeDirective(includeDirectiveToRemove);
		else if (min == singleLineCommentToRemove)
			removeComment(singleLineCommentToRemove, "\n");
		else if (min == multilineCommentToRemove)
			removeComment(multilineCommentToRemove, "*/\n");
	}

	if (outputFile[0] != '\0') {
		outputStream = fopen(outputFile, "w");
		if (outputStream == NULL)
			return;
	} else
		outputStream = stdout;

	fprintf(outputStream, "%s", buffer);
	fclose(outputStream);
	free(buffer);
}
