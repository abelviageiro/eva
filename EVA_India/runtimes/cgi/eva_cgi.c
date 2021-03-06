#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "amzi.h"

#define xTEST

// Converts nn to character
char x2c(char *what) 
{
    register char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return(digit);
}

// Converts plus signs to spaces
void plustospace(char *str) 
{
    register int x;

    for(x=0;str[x];x++) if(str[x] == '+') str[x] = ' ';
}

// Converts &nn's and %nn's to characters in url encoded strings
void unescape_url(char *url) 
{
    register int x,y;

    for(x=0,y=0;url[y];++x,++y) 
	{
        if((url[x] = url[y]) == '&') 
		{
            url[x] = x2c(&url[y+1]);
            y+=2;
        }
    }
    url[x] = '\0';
}

// Converts %nn's to characters in url encoded strings

void unpercent_url(char *url)
{
    register int x,y;

    for(x=0,y=0;url[y];++x,++y) 
	{
        if((url[x] = url[y]) == '%') 
		{
            url[x] = x2c(&url[y+1]);
            y+=2;
        }
    }
    url[x] = '\0';
}

void decode_url(char *url)
{
	plustospace(url);
	unescape_url(url);
	unpercent_url(url);
}

//   Any string of characters sent to Prolog is read using the
//   Prolog reader, which interprets a backslash as an escape
//   character.  So, this means if you really want a backslash,
//   such as in a file path, then it has to be a double
//   backslash.  This function makes that conversion.

void slashslash2(char* sout, const char* sin)
{
	while(*sin)
	{
		if (*sin == '\\')
		{
			*sout++ = *sin;
			*sout++ = *sin++;
		}
		else
			*sout++ = *sin++;
	}
	*sout = *sin;
	return;
}

char tokbuf[100];
char datetok[100];


char* build_date(char* sin)
{
	char *year, *month, *day;
	int iyear, imonth, iday;
	strncpy(tokbuf, sin, 99);
	strcpy(datetok, "date(");
	year = strtok(tokbuf, "-/");
	if (year == NULL) return "bad date";
	if (strlen(year) != 4) return "bad date";
	month = strtok(NULL, "-/");
	if (month == NULL) return "bad date";
	imonth = atoi(month);
	if (imonth < 1 || imonth > 12) return "bad date";
	day = strtok(NULL, "-/");
	if (day == NULL) return "bad date";
	iday = atoi(day);
	if (iday < 1 || iday > 31) return "bad date";
	strcat(datetok, year);
	strcat(datetok, ",");
	strcat(datetok, month);
	strcat(datetok, ",");
	strcat(datetok, day);
	strcat(datetok, ")");
	
	
	return datetok;
}


struct fact {
	char name[40];
	char value[80];
};

struct fact facts[100];


ENGid CurEng = NULL;
//FILE *outfile = stdout;

int main(int argc, char *argv[])
{
	char buffer[5000];
	char *val;
	char *buf, *buf2;
	long buflen;
	int i, j, n;
	char *p, *q, *tp;
	char valbuf[5000];
	char ibuf[10];
	int date_error = 0;
	
	// used for Prolog
	RC rc;
	TERM term;
	TF tf;
	char errmsg[1000];
	TERM table;
	TERM row;
	TERM item;
	
	val = getenv("CONTENT_LENGTH");
	if (val == NULL)
	{
		val = getenv("QUERY_STRING");
		if (val == NULL) return;
		buf = (char *)malloc(strlen(val)+2);
		strcpy(buf, val);
	}
	else
	{
		// Add two for two nuls and malloc space
		buflen = atol(val)+2;
		buf = (char *)malloc(buflen);
		if (buf == NULL) return;
		fgets(buf, buflen-1, stdin);
		buf[buflen-1] = '\0';			// Add an extra nul
	}
	
	//strcpy(buffer, head);
	//strcat(buffer, buf);
	//strcat(buffer, foot);
	//fputs(buffer, outfile);
	
	
	p = buf;
	i = 0;
	while (*p != '\0')
	{
		// Grab the attribute (up to the equal = sign)
		q = strchr(p, '=');
		if (q == NULL) // Not the right format
		{
			free(buf);
			return;
		}
		*q = '\0';
		tp = q + 1;
		decode_url(p);

		// get the attribute name
		strcpy(facts[i].name, p);
 		
		// Grab the value (up to the & or nul)
		p = tp;
		q = strchr(p, '&');
		if (q == NULL)		// End of string
		{
			q = &buf[buflen-1];
			tp = q;
		}
		else				// More values left to process
		{
			*q = '\0';
			tp = q + 1;
		}
		decode_url(p);
		
		if (strstr( facts[i].name, "Date" ) != 0 && strlen(p) > 0) {
			strcpy(facts[i].value, build_date(p));
			if (strcmp(facts[i].value, "bad date") == 0) date_error = 1;
		}
		else {
			strcpy(facts[i].value, p);
		}
		i++;

		p = tp;
	}
	
	n = i;
	
	
	printf("Content-type: text/html\n\n<html>\n\n<body><h1>EVA India</h1>");

#ifdef TEST
	strcpy(buffer, "<table border='3' cellpadding='5'>");
	for (i=0; i<n; i++) {
		strcat(buffer, "<tr>");
		strcat(buffer, "<td>");
		strcat(buffer, facts[i].name);
		strcat(buffer, "</td>");
		strcat(buffer, "<td>");
		strcat(buffer, facts[i].value);
		strcat(buffer, "</td>");
		strcat(buffer, "</tr>");
	}
	strcat(buffer, "</table>");
	fputs(buffer, stdout);  // was outfile
#endif
	free(buf);
	
	if (date_error == 1) {
		printf("<p><font color=red>Please enter dates in YYYY-MM-DD format</font></p>");
		goto main_error;
		}

	
	
	// Initialize the Prolog environment

#ifdef TEST
	printf("<P><font color='green'>lsInit cureng= %p</font></p>", CurEng);
#endif

	rc = lsInit(&CurEng, "arules");
	if (rc != 0)
	{
		lsGetExceptMsg(CurEng, errmsg, 1024);

		printf("<p><font color='red'>Fatal Error #%d initializing Amzi! Logic Server:\n%s</font></p>", rc, errmsg);
		goto main_error;
	}


#ifdef TEST
	printf("<P><font color='green'>lsInitLSX cureng= %p</font></p>", CurEng);
#endif
	rc = lsInitLSX(CurEng, NULL);
	if (rc != 0)
	{
		printf("<p><font color='red'>Fatal Error #%d initializing LSXs</font></p>", rc);
		goto main_error;
	}


	// Load the .xpl file 
#ifdef TEST
	printf("<P><font color='green'>lsLoad cureng= %p</font></p>", CurEng);
#endif
	rc = lsLoad(CurEng, "arules");
	if (rc != 0)
	{
		lsGetExceptMsg(CurEng, errmsg, 1024);

		printf("<p><font color='red'>Fatal Error #%d loading Amzi! Logic Server XPL file:\n%s</font></p>", rc, errmsg);
		goto main_error;
	}
	
#ifdef TEST
	printf("<P><font color='green'>lsExecStr version(X) cureng= %p</font></p>", CurEng);

	tf = lsExecStr(CurEng, &term, "version( ?x )");
	if (tf != TRUE)
	{
		printf("<p><font color='red'>version failed");
		goto main_error;
	}
	rc = lsGetArg(CurEng, term, 1, cSTR, buffer);
	printf("<P><font color='green'>version = %s, cureng= %p</font></p>", buffer, CurEng);
#endif
	
	
#ifdef TEST
	printf("<P><font color='green'>lsExecStr load('eva_india.axl') cureng= %p</font></p>", CurEng);
#endif
	tf = lsExecStr(CurEng, &term, "load('eva_india.axl')");
	if (tf != TRUE)
	{
		lsGetExceptMsg(CurEng, errmsg, 1024);

		printf("<p><font color='red'>Fatal Error #%d loading eva_india.axl file: %s</font></p>", rc, errmsg);
		goto main_error;
	}
	
#ifdef TEST
	printf("<P><font color='green'>lsExecStr arxl...  cureng= %p</font></p>", CurEng);
#endif
	// Assert the basic data for the case
	tf = lsExecStr(CurEng, &term, "arxl_initialize_table(`CommonRules`, `data`)");
	tf = lsExecStr(CurEng, &term, "arxl_initialize_table(`CommonRules`, `raw_vaccination`)");

	j = 1;
	for (i=0; i<n; i++) {
		if (strstr(facts[i].name, "Vaccination") != 0 && strlen(facts[i].value) > 0 ) {
			strcpy(buffer, "arxl_add_to_table(`CommonRules`, `raw_vaccination`, ");
			sprintf(ibuf, "%d", j);
			strcat(buffer, ibuf);
			strcat(buffer, ", `Vaccination`, `");
			strcat(buffer, facts[i].value);
			strcat(buffer, "`)");
			//printf(buffer);
			tf = lsExecStr(CurEng, &term, buffer);
			i++;
			strcpy(buffer,  "arxl_add_to_table(`CommonRules`, `raw_vaccination`, ");
			strcat(buffer, ibuf);
			strcat(buffer, ", `VaccinationDate`, `");
			strcat(buffer, facts[i].value);
			strcat(buffer, "`)");
			//printf(buffer);
			tf = lsExecStr(CurEng, &term, buffer);
			j++;
		}
		else {
			strcpy(buffer, "arxl_add_to_vector(`CommonRules`, `data`, `");
			strcat(buffer, facts[i].name);
			strcat(buffer, "`, `");
			strcat(buffer, facts[i].value);
			strcat(buffer, "`)");
			//printf(buffer);
			tf = lsExecStr(CurEng, &term, buffer);
		}
	}
	
	// Start the Output
	printf("<h2><font color=red>Demonstration Only, Not for Medical use</font></h2>");
	printf("<p><a href='/vaccinationanalysis/index.html'>EVA</a> is a tool for the encoding, verification, testing, and deployment of vaccination knowledge.</p>");
	printf("<p><a href='/vaccinationanalysis/eva_user_documentation.html'>Explanation of Results</a></p>");

	// Get the age
	tf = lsExecStr(CurEng, &term, "arxl_query('EVA', false, `FIND age`, ?x)");
	rc = lsGetArg(CurEng, term, 4, cSTR, buffer);
	printf("<h3>age = %s</h3>", buffer);
	// Get the history
	tf = lsExecStr(CurEng, &term, "arxl_query('EVA', false, `FIND history`, ?x)");
	rc = lsGetArg(CurEng, term, 4, cTERM, &table);
#ifdef TEST
	rc = lsTermToStr(CurEng, table, buffer, 500);
   if (strcmp(buffer, "[]") == 0) table = NULL;
	printf("<P><font color='green'>history = %s</font></p>", buffer);
#endif
	
	printf("<h2>Retrospective Analysis</h2>");
	printf("<p><font color=green>The comment indicates whether a previously given vaccination was:</p>");
	printf("<ul><li>too early,</li><li>early but OK,</li><li>during the optimal interval,</li><li>past the optimal interval,</li>");
	printf("<li>a violation of minimum intervals between doses, or</li><li>an invalid spacing between live virus vaccinations.</li></ul>");
	printf("<p>A dose with an X was not given at the correct time and does not count as a valid dose. </font></p>");
	printf("<table border='3' cellspacing='2' cellpadding='5'>");
	printf("<tr>");
	printf("<th>Vaccination</th>");
	printf("<th>Dose</th>");
	printf("<th>Date Given</th>");
	printf("<th>Status</th>");
	printf("<th>Comment</th>");
	printf("<th>Age Given</th>");
	printf("</tr>");

	i = 0;
	while (table != 0 && i < 50) {
		lsGetHead(CurEng, table, cTERM, &row);
		j = 0;
		printf("<tr>");
		while (row != 0 && j < 10) {
			lsGetHead(CurEng, row, cTERM, &item);
			lsTermToStr(CurEng, item, buffer, 80);
			printf("<td>%s</td>", buffer);
			row = lsGetTail(CurEng, row);
			j++;
			}
		table = lsGetTail(CurEng, table);
		printf("</tr>");
		i++;
		}
	
	printf("</table>");

	// Get the plan
	tf = lsExecStr(CurEng, &term, "arxl_query('EVA', false, `FIND plans`, ?x)");
	rc = lsGetArg(CurEng, term, 4, cTERM, &table);
#ifdef TEST
	rc = lsTermToStr(CurEng, table, buffer, 500);
	printf("<P><font color='green'>plans = %s</font></p>", buffer);
#endif
	
	printf("<h2>Future Plan</h2>");
	printf("<p><font color=green>The status indicates what should happen next for a given vaccine as of the date the report is given.  The results are sorted with most critical future vaccinations first.  Possible values are:</p>");
	printf("<ul>");
	printf("<li>not applicable - for example doctor deferred or HPV for males.</li>");
	printf("<li>complete - the full series has been given.</li>");
	printf("<li>current - the vaccinations are up-to-date, nothing to do today.</li>");
	printf("<li>eligible - it's after the minimum date, so it could be given today, but it's before the optimal range.</li>");
	printf("<li>due - it's in the optimal range.</li>");
	printf("<li>past due - it's past the optimal range.</li>");
	printf("</ul>");
	printf("<p>The three dates indicate the earliest date for the next dose, and the beginning and end of the optimal range. ");
	printf("Note that the <a href='http://www.iapindia.org/immunisation/immunisation-schedule'>IAP ITT</a> documents do not specify a minimum age or interval, so for this demonstration the earliest age and the optimal start are the same.");
	printf(" The document does not specify the acceptable range either, so either one week or one month was assumed.");
	printf("</font></p>");
	
	printf("<table border='3' cellspacing='2' cellpadding='5'>");
	printf("<tr>");
	printf("<th>Vaccination</th>");
	printf("<th>Status</th>");
	printf("<th>Dose</th>");
	printf("<th>Earliest</th>");
	printf("<th>Optimal Start</th>");
	printf("<th>Optimal End</th>");
	printf("<th>Citation</th>");
	printf("<th>Comment</th>");
	printf("</tr>");

	i = 0;
	while (table != 0 && i < 50) {
		lsGetHead(CurEng, table, cTERM, &row);
		j = 0;
		printf("<tr>");
		while (row != 0 && j < 10) {
			lsGetHead(CurEng, row, cTERM, &item);
			lsTermToStr(CurEng, item, buffer, 80);
			printf("<td>%s</td>", buffer);
			row = lsGetTail(CurEng, row);
			j++;
			}
		table = lsGetTail(CurEng, table);
		printf("</tr>");
		i++;
		}
	
	printf("</table>");


		
main_error:

	rc = lsClose(CurEng);
	
	printf("<p>Use the browser back button to enter another.</p></body></html>");
	
	return(TRUE);
}

