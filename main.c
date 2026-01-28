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
	static int winANSI[32] = 
	{
		0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
	    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
	    0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178
	};

	static int macRoman[128] = 
	{
		0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
	    0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
	    0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
	    0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
	
	    0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
	    0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
	    0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
	    0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
	
	    0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
	    0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
	    0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
	    0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
	
	    0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
	    0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
	    0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
	    0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
	};

	for(int i = 0; i < 128; i++) encoding[i] = i;
	for(int i = 160; i < 256; i++) encoding[i] = i;
	memcpy(encoding + 128, winANSI, sizeof(winANSI));

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
		memcpy(encoding + 128, macRoman, sizeof(macRoman));

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
