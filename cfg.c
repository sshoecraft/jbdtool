
/* uncomment this if you have conv.c & conv.h */
//#define HAVE_CONF

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "cfg.h"
#include "utils.h"
#ifdef HAVE_CONF
#include "conv.h"
#endif

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#if DEBUG
#define dprintf(format, args...) { printf("%s(%d): " format "\n",__FUNCTION__,__LINE__, ## args); fflush(stdout); }
#else
#define dprintf(format, args...) /* noop */
#endif

char *typenames[] = { "Unknown","char","string","byte","short","int","long","quad","float","double","logical","date","list" };

#ifndef HAVE_CONV
static void conv_type(int dtype,void *dest,int dlen,int stype,void *src,int slen) {
	char temp[1024];
	int *iptr;
	float *fptr;
	double *dptr;
	char *sptr;
	int len;

	dprintf("stype: %d, src: %p, slen: %d, dtype: %d, dest: %p, dlen: %d",
		stype,src,slen,dtype,dest,dlen);

	dprintf("stype: %s, dtype: %s\n", typenames[stype],typenames[dtype]);
	switch(dtype) {
	case DATA_TYPE_INT:
		iptr = dest;
		*iptr = atoi(src);
		break;
	case DATA_TYPE_FLOAT:
	case DATA_TYPE_DOUBLE:
		fptr = dest;
		*fptr = atof(src);
		break;
	case DATA_TYPE_LOGICAL:
		dprintf("src: %s\n", (char *)src);
		/* yes/no/true/false */
		{
			char temp[16];
			int i;

			iptr = dest;
			i = 0;
			for(sptr = src; *sptr; sptr++) temp[i++] = tolower(*sptr);
			temp[i] = 0;
			dprintf("temp: %s\n", temp);
			if (strcmp(temp,"yes") == 0 || strcmp(temp,"true") == 0 || strcmp(temp,"1")==0)
				*iptr = 1;
			else
				*iptr = 0;
			dprintf("iptr: %d\n", *iptr);
		}
		break;
	case DATA_TYPE_STRING:
		switch(stype) {
		case DATA_TYPE_INT:
			iptr = src;
			sprintf(temp,"%d",*iptr);
			break;
		case DATA_TYPE_FLOAT:
			fptr = src;
			sprintf(temp,"%f",*fptr);
			break;
		case DATA_TYPE_DOUBLE:
			dptr = src;
			sprintf(temp,"%f",*dptr);
			break;
		case DATA_TYPE_LOGICAL:
			iptr = src;
			sprintf(temp,"%s",*iptr ? "yes" : "no");
			break;
		case DATA_TYPE_STRING:
			temp[0] = 0;
			len = slen > dlen ? dlen : slen;
			strncat(temp,src,len);
			break;
		default:
			printf("conv_type: unknown src type: %d\n", stype);
			temp[0] = 0;
			break;
		}
		dprintf("temp: %s\n",temp);
		strcpy((char *)dest,temp);
		break;
	default:
		printf("conv_type: unknown dest type: %d\n", dtype);
		break;
	}
}
#endif

CFG_INFO *cfg_create(char *filename) {
	CFG_INFO *cfg_info;

	/* Allocate memory for the new cfg_info */
	cfg_info = (CFG_INFO *) malloc(CFG_INFO_SIZE);
	if (!cfg_info) {
		return 0;
	}

	/* Save filename */
	strcpy(cfg_info->filename,filename);

	/* Create the sections list */
	cfg_info->sections = list_create();

	/* Return the new cfg_info */
	return cfg_info;
}

CFG_SECTION *cfg_create_section(CFG_INFO *info,char *name) {
	struct _cfg_section newsec,*section;
	register char *ptr;
	register int x;


	/* Add the name and create the lists */
	newsec.name[0] = x = 0;
	for(ptr=name; *ptr != 0 && x < sizeof(newsec.name); ptr++)
		newsec.name[x++] = toupper(*ptr);
	newsec.name[x] = 0;
	newsec.items = list_create();

	/* Add the new section to the cfg_info */
	section = list_add(info->sections,&newsec,CFG_SECTION_SIZE);
	if (!section) {
		list_destroy(newsec.items);
		return 0;
	}

	/* Return the new section */
	return section;
}

void cfg_destroy(CFG_INFO *info) {
	CFG_SECTION *section;

	/* Try and fool me! Ha! */
	if (!info) return;

	/* Destroy each section's items */
	list_reset(info->sections);
	while( (section = list_get_next(info->sections)) != 0)
		list_destroy(section->items);

	/* Destroy the sections list */
	list_destroy(info->sections);

	/* Now free the cfg_info */
	free(info);

	return;
}

CFG_ITEM *_cfg_get_item(CFG_SECTION *section, char *name) {
	CFG_ITEM *item;

	dprintf("_cfg_get_item: looking for keyword: %s", name);
	list_reset(section->items);
	while( (item = list_get_next(section->items)) != 0) {
		dprintf("item->keyword: %s",item->keyword);
		if (strcmp(item->keyword,name)==0) {
			dprintf( "_cfg_get_item: found\n");
			return item;
		}
	}
	dprintf( "_cfg_get_item: not found");
	return 0;
}

char *cfg_get_item(CFG_INFO *info,char *section_name,char *keyword) {
	CFG_SECTION *section;
	CFG_ITEM *item;
	char itemname[CFG_KEYWORD_SIZE];
	register int x;

	if (!info) return 0;
	dprintf("cfg_get_item: section name: %s, keyword: %s", section_name,keyword);

	/* Get the section */
	section = cfg_get_section(info,section_name);
	if (!section) return 0;

	/* Convert the keyword to uppercase */
	for(x=0; keyword[x] != 0; x++) itemname[x] = toupper(keyword[x]);
	itemname[x] = 0;

	/* Get the item */
	item = _cfg_get_item(section, itemname);
	if (item) return item->value;
	return 0;
}

char *cfg_get_string(CFG_INFO *info, char *name, char *keyword, char *def) {
	char *p;

	p = cfg_get_item(info, name, keyword);
	return (p ? p : def);
}

int cfg_get_bool(CFG_INFO *info, char *name, char *keyword, int def) {
	char *p;

	p = cfg_get_item(info, name, keyword);
	if (!p) return def;
	if (*p == '1' || *p == 't' || *p == 'T' || *p == 'y' || *p == 'Y')
		return 1;
	else
		return 0;
}

int cfg_get_int(CFG_INFO *info, char *name, char *keyword, int def) {
	char *p;
	int val;

	p = cfg_get_item(info, name, keyword);
	if (!p) return def;
	val = atoi(p);
	conv_type(DATA_TYPE_INT,&val,0,DATA_TYPE_STRING,p,strlen(p));
	return val;
}

long long cfg_get_quad(CFG_INFO *info, char *name, char *keyword, long long def) {
	char *p;
	long long val;

	p = cfg_get_item(info, name, keyword);
	if (!p) return def;
	val = strtoll(p,0,10);
	conv_type(DATA_TYPE_QUAD,&val,0,DATA_TYPE_STRING,p,strlen(p));
	return val;
}

double cfg_get_double(CFG_INFO *info, char *name, char *keyword, double def) {
	char *p;
	double val;

	p = cfg_get_item(info, name, keyword);
	if (!p) return def;
	conv_type(DATA_TYPE_DOUBLE,&val,0,DATA_TYPE_STRING,p,strlen(p));
	return val;
}

list cfg_get_list(CFG_INFO *info, char *name, char *keyword, char *def) {
	char *str,*p;
	list l;
	int i;

	str = cfg_get_item(info, name, keyword);
	if (!str) str = def;
	l = list_create();
	i = 0;
	while(1) {
		p = strele(i++,",",str);
		if (!strlen(p)) break;
		list_add(l,p,strlen(p)+1);
	}

	return l;
}

int cfg_set_item(CFG_INFO *info, char *sname, char *iname, char *desc, char *ival) {
	CFG_SECTION *section;
	CFG_ITEM *item,newitem;;
	char itemname[CFG_KEYWORD_SIZE-1];
	register int x;

	if (!info) return 1;

	/* Get the section */
	dprintf("cfg_set_item: section: %s, name: %s, value: %s", sname,iname,ival);
	section = cfg_get_section(info,sname);
	if (!section) {
		dprintf("cfg_set_item: creating %s section", sname);
		section = cfg_create_section(info,sname);
		if (!section) {
			perror("cfg_create_section");
			return 1;
		}
	}

	/* Convert the keyword to uppercase */
	for(x=0; iname[x] != 0; x++) itemname[x] = toupper(iname[x]);
	itemname[x] = 0;

	item = _cfg_get_item(section, itemname);
	if (item) {
		free(item->value);
		item->value = (char *) malloc(strlen(ival)+1);
		if (!item->value) {
			perror("cfg_set_item: malloc repl");
			return 1;
		}
		strcpy(item->value,ival);
		dprintf("cfg_set_item: desc: %p", desc);
		if (desc) {
			dprintf("cfg_set_item: desc: %s", desc);
			dprintf("cfg_set_item: item->desc: %p", item->desc);
			if (item->desc) {
				dprintf("cfg_set_item: item->desc: %s", item->desc);
				free(item->desc);
			}
			item->desc = (char *) malloc(strlen(desc)+1);
			strcpy(item->desc,desc);
		}
	} else {
		/* Add a new item */
		memset(&newitem,0,sizeof(newitem));
		strncpy(newitem.keyword, itemname, sizeof(newitem.keyword)-1);
		newitem.value = (char *) malloc(strlen(ival)+1);
		if (!newitem.value) {
			perror("cfg_set_item: malloc new");
			return 1;
		}
		strcpy(newitem.value,ival);
		if (desc) {
			newitem.desc = (char *) malloc(strlen(desc)+1);
			strcpy(newitem.desc,desc);
		}
		list_add(section->items,&newitem,CFG_ITEM_SIZE);
	}

	return 0;
}

int cfg_set_bool(CFG_INFO *info, char *name, char *keyword, int val) {
	char *p;

	p = (val ? "1" : 0);
	return cfg_set_item(info, name, keyword, 0, p);
}

int cfg_set_int(CFG_INFO *info, char *name, char *keyword, int val) {
	char ival[16];

	sprintf(ival,"%d",val);
	return cfg_set_item(info, name, keyword, 0, ival);
}

int cfg_set_quad(CFG_INFO *info, char *name, char *keyword, long long val) {
	char ival[32];

	sprintf(ival,"%lld",val);
	return cfg_set_item(info, name, keyword, 0, ival);
}

int cfg_set_double(CFG_INFO *info, char *name, char *keyword, double val) {
	char ival[32]; 

	sprintf(ival,"%lf",val);
	return cfg_set_item(info, name, keyword, 0, ival);
}

extern CFG_ITEM *_cfg_get_item(CFG_SECTION *,char *);

static char *_readline(char *line,int size,FILE *fp) {
	int have_start;
	char *line_end;
	register char *ptr;
	register int ch;

	/* If eof, don't bother */
	if (feof(fp)) return 0;

	ptr = line;
	line_end = line + (size-1);
	have_start = 0;
	line[0] = 0;
	while( (ch = fgetc(fp)) != EOF) {
		/* We only accept ascii chars, thank you... */
		if (!isascii(ch)) continue;
		/* We need to see blank lines... */
		if (ch == '\n') break;
		if (have_start) {
			if (ptr < line_end)
				*ptr++ = ch;
			else
				break;
		}
		else {
			if (!isspace(ch)) {
				have_start = 1;
				*ptr++ = ch;
			}
		}
	}
	/* Read anything? */
	if (ptr > line) {
		/* Trim up the end and return it */
		while(isspace((int)*(ptr-1))) ptr--;
		*ptr = 0;
	}
	return line;
}

CFG_INFO *cfg_read(char *filename) {
	FILE *fp;
	CFG_INFO *cfg_info;
	CFG_SECTION *section;
	CFG_ITEM *item,newitem;
	char line[1022],name[CFG_SECTION_NAME_SIZE],*ts_ptr,*te_ptr;
	char desc[1024];
	register char *ptr;
	register int x;

	dprintf("cfg_read: filename: %s",filename);
	fp = fopen(filename,"r");
	if (!fp) {
		dprintf("cfg_read: error: fopen(%s): %s", filename, strerror(errno));
		return 0;
	}

	/* Create a cfg_info with no type (version defaults to 2) */
	cfg_info = cfg_create(filename);
	if (!cfg_info) {
		fclose(fp);
		return 0;
	}

	/* Ensure we set section to null */
	section = 0;

	/* Read all lines */
	desc[0] = 0;
	while(_readline(line,sizeof(line),fp)) {
		dprintf("_cfg_read: line: %s",line);

		/* Blank lines end a section */
		if (!strlen(line)) {
			if (section) section = 0;
			continue;
		}

		/* Is there a LB? MUST be at start of line... */
		ts_ptr = (line[0] == '[' ? line : 0);

		/* If there's a LB see if there's a RB */
		te_ptr = (ts_ptr ? strchr(line,']') : 0);

		/* If we have both open and close... */
		if (ts_ptr && te_ptr) {
			/* Get the section name and create it */
			x = 0;
			for(ptr = ts_ptr+1; ptr < te_ptr; ptr++) {
				name[x++] = toupper(*ptr);
				if (x > sizeof(name)-2) break;
			}
			name[x] = 0;
			section = cfg_get_section(cfg_info,name);
			if (!section) section = cfg_create_section(cfg_info,name);
			desc[0] = 0;
		}
		else {
			/* Do we have a section? */
			if (!section) continue;

			/* Is this line commented out? */
			if (line[0] == ';' || line[0] == '#') {
				desc[0] = 0;
				strncat(desc,line,sizeof(desc)-1);
				continue;
			}

			/* Init newitem */
			memset(&newitem,0,sizeof(newitem));

			/* Get the keyword */
			ptr=line;
			newitem.keyword[0] = x = 0;
			while(*ptr != '=' && *ptr != 0) {
				newitem.keyword[x++] = toupper(*ptr++);
				if (x > sizeof(newitem.keyword)-2) break;
			}
			newitem.keyword[x] = 0;
			trim(newitem.keyword);

			/* Get the value (little tricky-dicky here) */
			ptr++;
			newitem.value = (char *) malloc(strlen(ptr)+1);
			strcpy(newitem.value,ptr);

			/* Set desc/comment */
			if (desc[0]) {
				newitem.desc = (char *) malloc(strlen(desc)+1);
				strcpy(newitem.desc,desc);
			}

			/* Was there a prev item for this keyword? */
			item = _cfg_get_item(section,newitem.keyword);
			if (item) {
				dprintf("cfg_read: deleting previous item!");
				list_delete(section->items,item);
			}


			/* Add the new item to the section */
			dprintf("cfg_read: keyword: %s, value: %s", newitem.keyword,newitem.value);

			list_add(section->items,&newitem,CFG_ITEM_SIZE);
			desc[0] = 0;
		}
	}
	fclose(fp);

	cfg_info->filename[0] = 0;
	strncat(cfg_info->filename, filename, sizeof(cfg_info->filename)-1);
	return cfg_info;
}

CFG_SECTION *cfg_get_section(CFG_INFO *info,char *name) {
	CFG_SECTION *section;
	char secname[CFG_SECTION_NAME_SIZE];
	register int x;

	if (!info) return 0;

	/* Convert the section name to uppercase */
	for(x=0; name[x] != 0; x++) secname[x] = toupper(name[x]);
	secname[x] = 0;

	/* Find the section */
	dprintf("cfg_get_section: looking for section: %s", secname);
	list_reset(info->sections);
	while( (section = list_get_next(info->sections)) != 0) {
		dprintf("name: %s", section->name);
		if (strcmp(section->name,secname)==0) {
			dprintf("found section.");
			return section;
		}
	}
	return 0;
}

int cfg_get_tab(CFG_INFO *info, struct cfg_proctab *tab) {
	struct cfg_proctab *ent;
	char *p;
	int i;

	dprintf("cfg_get_tab: filename: %s",info->filename);
	for(i=0; tab[i].section; i++) {
		ent = &tab[i];
		p = cfg_get_item(info, ent->section, ent->keyword);
		dprintf("p: %p, ent->def: %p", p, ent->def);
		if (!p) {
			if (ent->def)
				p = ent->def;
			else 
				continue;
		}
		dprintf("cfg_get_tab: section: %s, keyword: %s, value: %s", ent->section, ent->keyword, p);
		conv_type(ent->type,ent->dest,ent->dlen,DATA_TYPE_STRING,p,strlen(p));
	}

	return 0;
}

#if 0
int cfg_set_tab(CFG_INFO *info, struct cfg_proctab *tab, int empty) {
	struct cfg_proctab *ent;
	char temp[4096];
	int i;

	for(i=0; tab[i].section; i++) {
		ent = &tab[i];
		conv_type(DATA_TYPE_STRING,temp,sizeof(temp),ent->type,ent->dest,ent->dlen);
		dprintf("cfg_set_tab: section: %s, keyword: %s, value: %s\n",
			ent->section, ent->keyword, temp);
		if (empty || strlen(temp)) cfg_set_item(info,ent->section,ent->keyword,ent->desc,temp);
	}
	return 0;
}
#endif

void cfg_disp_tab(struct cfg_proctab *tab, char *name, int logopt) {
	char temp[1024],stemp[CFG_SECTION_NAME_SIZE],format[32];
	struct cfg_proctab *ent;
	int smax,kmax;

	/* Get len of longest section */
	smax = 0;
	for(ent = tab; ent->section; ent++) {
		if (strlen(ent->section) > smax)
			smax = strlen(ent->section);
	}
	/* Get len of longest keyword */
	kmax = 0;
	for(ent = tab; ent->section; ent++) {
		if (strlen(ent->keyword) > kmax)
			kmax = strlen(ent->keyword);
	}
	dprintf("smax: %d, kmax: %d", smax,kmax);
	sprintf(format,"%%-%ds %%%ds: %%s\n",smax+4, kmax+4);
	dprintf("format: %s", format);

	/* Display the tab */
	dprintf("cfg_disp_tab: displaying tab...");
	dprintf("*************** %s Configuration ***************", name);
	for(ent = tab; ent->section; ent++) {
		dprintf("ent->section: %s", ent->section);
		dprintf("keyword: %s", ent->keyword);
		conv_type(DATA_TYPE_STRING,temp,sizeof(temp),ent->type,ent->dest,ent->dlen);
		dprintf("temp: %s", temp);
		sprintf(stemp,"[%s]",ent->section);
		printf(format,stemp,ent->keyword,temp);
	}
	dprintf("cfg_disp_tab: done!");
}

#if 0
static int cmpsec(const void *i1, const void *i2) {
	const struct cfg_proctab *p1 = i1;
	const struct cfg_proctab *p2 = i2;

	return strcmp(p1->section,p2->section);
}
#endif

#if 0
struct cfg_proctab *cfg_combine_tabs(struct cfg_proctab *tab1, struct cfg_proctab *tab2) {
	char *p;
	struct cfg_proctab *tab,*ent;
	int count,i,found;
	list l;

	dprintf("cfg_combine_tabs: tab1: %p, tab2: %p", tab1, tab2);

	l = list_create();

	/* Get a count of both tabs */
	count = 0;
	if (tab1) {
		for(ent = tab1; ent->section; ent++) {
			found = 0;
			list_reset(l);
			while( (p = list_get_next(l)) != 0) {
				if (strcmp(ent->section,p) == 0) {
					found = 1;
					break;
				}
			}
			if (!found) list_add(l,ent->section,strlen(ent->section)+1);
			count++;
		}
	}
	if (tab2) {
		for(ent = tab2; ent->section; ent++) {
			found = 0;
			list_reset(l);
			while( (p = list_get_next(l)) != 0) {
				if (strcmp(ent->section,p) == 0) {
					found = 1;
					break;
				}
			}
			if (!found) list_add(l,ent->section,strlen(ent->section)+1);
			count++;
		}
	}
	dprintf("cfg_combine_tabs: count: %d", count);

	/* Alloc a new tab big enough to hold both XXX plus end marker  */
	tab = (struct cfg_proctab *) malloc(sizeof(struct cfg_proctab) * (count + 1));
	if (!tab) return 0;
	i = 0;

	/* list is a list of sections *IN THE ORDER WE FOUND THEM* */
	list_reset(l);
	while( (p = list_get_next(l)) != 0) {
		if (tab1) {
			for(ent = tab1; ent->section; ent++) {
				if (strcmp(ent->section,p) == 0)
					memcpy(&tab[i++],ent,sizeof(*ent));
			}
		}
		if (tab2) {
			for(ent = tab2; ent->section; ent++) {
				if (strcmp(ent->section,p) == 0)
					memcpy(&tab[i++],ent,sizeof(*ent));
			}
		}
	}
	memset(&tab[i],0,sizeof(*ent));
	list_destroy(l);

	dprintf("cfg_combine_tabs: returning: %p",tab);
	return tab;
}
#endif


int cfg_write(CFG_INFO *info) {
	FILE *fp;
	CFG_SECTION *section;
	CFG_ITEM *item;

	/* open the file */
	dprintf("cfg_write: filename: %s", info->filename);
	fp = fopen(info->filename,"wb+");
	if (!fp) return 1;

	/* For each section, write the items out */
	list_reset(info->sections);
	while( (section = list_get_next(info->sections)) != 0) {
		/* Write the section name */
		fprintf(fp,"[%s]\n",section->name);

		/* Write the section's data */
		list_reset(section->items);
		while( (item = list_get_next(section->items)) != 0) {
			if (item->desc) {
				if (item->desc[0] != ';') fprintf(fp,";");
				fprintf(fp,"%s\n",item->desc);
			}
			fprintf(fp,"%s=%s\n",item->keyword,item->value);
		}

		/* Write a blank line to end the section */
		fprintf(fp,"\n");
	}
	fclose(fp);

	return 0;
}
