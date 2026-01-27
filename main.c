#include <pdfio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "name_map.h"

static inline size_t len(const char *str)
{
	return strlen(str) + 1;
}

static void	load_encoding(pdfio_obj_t *page_obj, const char *name, int encoding[256]);
static void	put_utf8(int ch);
static void	puts_utf16(const char *s);

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
		outputName = (char*)calloc(len(filename), sizeof(char));
		strcpy(outputName, filename);
		outputName[len(argv[1])-2] = '2';
		outputName[len(argv[1])-3] = 'b';
		outputName[len(argv[1])-4] = 'f';
	}
	else 
	{
		outputName = (char*)calloc(len(argv[2]), sizeof(char));
		strcpy(outputName, argv[2]);
	}

	FILE *output = fopen(outputName, "w");
	free(outputName);

	if(output == NULL)
	{
		fputs("Unable to create .fb2 file\n", stderr);
		return 1;
	}

	//reading metadata
	const char *author = pdfioFileGetAuthor(pdf);
	char *title = (char*)calloc(len(argv[1]), sizeof(char));
	strcpy(title, argv[1]);
	title[len(argv[1])-5] = '\0';

	//writing metadata
	fprintf(output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(output, "<FictionBook>\n<description>\n\t<title-info>\n");	
	fprintf(output, "\t\t<genre></genre>\n\t\t<author>%s</author>\n", author);
	fprintf(output, "\t\t<book-title>%s</book-title>\n\t</title-info>\n</description>\n", title);

	free(title);

	//reading pdf
	pdfio_obj_t *page;
	pdfio_stream_t *stream;
	char buffer[1024];
	bool in = 0, first = 1;
	int encoding[256];

	/*
	for(size_t i = 0, numPages = pdfioFileGetNumPages(pdf); i < numPages; i++)
	{
		page = pdfioFileGetPage(pdf, i);

		while(pdfioStreamGetToken(stream, buffer, sizeof(buffer)))
		{
			if(!strcmp(buffer, "[")) in = 1;
			else if(!strcmp(buffer, "]")) in = 0;


		}
	}
*/
	fclose(output);
	
	pdfioFileClose(pdf);

	return 0;
}
