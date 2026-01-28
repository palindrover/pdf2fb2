#include <pdfio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "name_map.h"

static inline size_t len(const char *str)
{
	return strlen(str) + 1;
}

static void	load_encoding(pdfio_obj_t *page_obj, const char *name, int encoding[256]);
static void	put_utf8(int ch, FILE *stream);
static void	puts_utf16(const char *s, FILE *stream);

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		fputs("Specify input file\n", stderr);
		return 1;
	}

	const char *filename = argv[1];
	pdfio_file_t *pdf = pdfioFileOpen(filename, NULL, NULL, NULL, NULL);
	
	char *outputName;
	if(argc < 3)
	{
		outputName = argv[1];
		outputName[len(argv[1])-2] = '2';
		outputName[len(argv[1])-3] = 'b';
		outputName[len(argv[1])-4] = 'f';
	}
	else strcpy(outputName, argv[2]);

	FILE *output = fopen(outputName, "w");

	if(output == NULL)
	{
		fputs("Unable to create .fb2 file\n", stderr);
		return 1;
	}

	//reading metadata
	const char *author = pdfioFileGetAuthor(pdf);
	char *title = argv[1];
	title[len(argv[1])-5] = '\0';

	//writing metadata
	fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(output, "<FictionBook>\n<description>\n\t<title-info>\n");	
	fprintf(output, "\t\t<genre></genre>\n\t\t<author>%s</author>\n", author);
	fprintf(output, "\t\t<book-title>%s</book-title>\n\t</title-info>\n</description>\n", title);

	//reading pdf
	pdfio_obj_t *page;
	pdfio_stream_t *stream;
	char buffer[1024], *bufptr, font[256];
	bool in = 0, first = 1;
	int encoding[256];

	fprintf(output, "<body>\n");	

	for(size_t i = 0, numPages = pdfioFileGetNumPages(pdf); i < numPages; i++)
	{
		page = pdfioFileGetPage(pdf, i);

		while(pdfioStreamGetToken(stream, buffer, sizeof(buffer)))
		{
			if(!strcmp(buffer, "[")) in = 1;
			else if(!strcmp(buffer, "]")) in = 0;
			else if(!first && in && (isdigit(buffer[0]) || buffer[0] == '-') && fabs(atof(buffer)) > 100) putchar(' ');
			else if(buffer[0] == '(')
			{
				first = 0;
				for(bufptr = buffer + 1; *bufptr; bufptr++) put_utf8(encoding[*bufptr & 255], output);
			}
			else if(buffer[0] == '<')
			{
				first = 0;
				puts_utf16(buffer + 1, output);
			}
			else if(buffer[0] == '/')
			{
				strncpy(font, buffer + 1, sizeof(font) - 1);
				font[sizeof(font) - 1] = '\0';
			}
			else if(!strcmp(buffer,"Tf") && font[0]) load_encoding(page, font, encoding);
			else if(!strcmp(buffer, "Td") || !strcmp(buffer, "TD") || !strcmp(buffer, "T*") || !strcmp(buffer, "\'") || !strcmp(buffer, "\""))
			{
				putchar('\n');
				first = 1;
			}
		}
	}

	fclose(output);
	
	pdfioFileClose(pdf);

	return 0;
}

static void load_encoding(pdfio_obj_t *pageObj, const char *font, int encoding[256])
{
	pdfio_dict_t *pageDict, *resourcesDict, *fontDict;
	pdfio_obj_t *fontObj, *encodingObj;
	static int win_ansi[32];
	for(int i  = 128; i <= 159; i++) win_ansi[i-128] = i;
	static int mac_roman[128];
	for(int i = 128; i <= 255; i++) mac_roman[i-128] = i;

	for(int i = 0; i < 128; i++) encoding[i] = i;
	for(int i = 160; i < 256; i++) encoding[i] = i;
	memcpy(encoding + 128, win_ansi, sizeof(win_ansi));

	if((pageDict = pdfioObjGetDict(pageObj)) == NULL) return;
	if((resourcesDict = pdfioDictGetDict(pageDict, "Resources")) == NULL) return;
	if((fontDict = pdfioDictGetDict(resourcesDict, "Font")) == NULL)
	{
		if((fontObj = pdfioDictGetObj(resourcesDict, "Font")) == NULL) 
			fontDict = pdfioObjGetDict(fontObj);
		if(!fontDict) return;
	}
	if((fontObj = pdfioDictGetObj(fontDict, font)) == NULL) return;

	pdfio_dict_t *encodingDict;

	if((encodingObj = pdfioDictGetObj(pdfioObjGetDict(fontObj), "Encoding")) == NULL) return;
	if((encodingDict = pdfioObjGetDict(encodingObj)) == NULL) return;

	const char *baseEncoding;
	pdfio_array_t *differences;

	baseEncoding = pdfioDictGetName(encodingDict, "BaseEncoding");
	differences = pdfioDictGetArray(encodingDict, "Differences");

	if(baseEncoding && !strcmp(baseEncoding, "MacRomanEncoding")) 
		memcpy(encoding + 128, mac_roman, sizeof(mac_roman));

	if(differences)
	{
		size_t count = pdfioArrayGetSize(differences);
		const char *name;
		size_t idx = 0;

		for(size_t i = 0; i < count; i++)
		{
			switch (pdfioArrayGetType(differences, i)) 
			{
				case PDFIO_VALTYPE_NUMBER:
					idx = (size_t)pdfioArrayGetNumber(differences, i);
					break;
				
				case PDFIO_VALTYPE_NAME:
					if(idx < 0 || idx > 255) break;

					name = pdfioArrayGetName(differences, i);
					for(size_t j=0; j< (sizeof(unicode_map)/sizeof(unicode_map[0])); j++)
						if(!strcmp(name, unicode_map[j].name))
						{
							encoding[idx] = unicode_map[j].unicode;
							break;
						}
					idx++;
					break;
				
				default: break;
			}
		}
	}
}
