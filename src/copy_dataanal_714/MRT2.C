/* to compile, using Mark Williams CC86
modified for Lattice c
cc a:mrt.c libm.olb -v -xtm */

/* modified to allow min and max conds */

/* allows exceptions file of form  item#  +/-condition#adjustment */


/*  modified for PC using Turbo-C September 17, 1987 */
/* modified to permit pulling cond #s for questions from previous line */
/* using ncmin and ncmax for the limits on the current trial's condition number */
/* and ncminl and ncmaxl, the presumed limits for the previous trial, */
/* as the limits for actually storing and reporting data*/
/* 12-89 */
/* 1/91 */
/* SPECIAL VERSION FOR VARIANCES */

void main(void);
void sort(int nf,int median,int ccpl);
void oops(char *string);
void writemeans(int ns);
void openfail(char *filestr);
int correct(int *cutoff,float *sigma,int *truncelim,char *sfname,char *msfname,
char *ifname,char *mifname,char *spname,char *mspname,char *ipname,
char *mipname,int *ccpl,int *icpl,int *median);
int filtsub(int cutoff,float sigma,int truncelim,char *filename,int spcon,int exceptflag);

#include "stdio.h"
#include "io.h"
#include "stdlib.h"
#include "math.h"
#include "ctype.h"
#include "alloc.h"
#include "dos.h"
#define ERR -1
#define MAXLINE 512
#define MAXLINEX 1024
#define	EQ	== 

/* externals */
FILE *printer,*subfile,*msubfile,*psubfile,*pmsubfile;
char *tgets();
int order();
struct trial
	{
	int cond;
	int lcond;
	int item;
	int rt;
	int subclass;
	};

struct trial trials[200];
char file[MAXLINE];
char buff[MAXLINEX];
int *nmat,*nrejtot,*nsum,*nrejsum,*inmat,*insum,*cntsum,*corrsum,*icntsum,*icorrsum;
long int *totmat,*totsum,*itotmat,*itotsum;
double *sumsq,*isumsq;
int *items_array;	/* array of items x conds x trials */
int *items_count_array;	/* array of counts of items x conds */
int *mdnrts;
int ncmax,ncmin;
int nimax,nimin;
int ncmaxl,ncminl;
int last_cond;
int nc,ni,nsc,nsca,corrsc,cval,ival,rtval,scval;
int sc[20];
int DEBUG;
int vexcept[400];
int lowcutoff;

/* main program */

void main()
{
char sfname[20],ifname[20],msfname[20],mifname[20]; /* names for RT files */
char spname[20],ipname[20],mspname[20],mipname[20]; /* names for prop files */
char sfile[MAXLINE];
FILE *control;
FILE *efp;
char temp[80];
struct date today;
int i,ns,cutoff,median,n,truncelim,ccpl,icpl;
float sigma;
int spcon;
int exceptflag,ei,ej;
int c;
int printer_control;
char tch[20];

printf("\n\nMRT version of January 7, 1991");
printf("\nSPECIAL VERSION FOR VARIANCES IN SUB AND ITEM 'RT' FILES.");
printf("\nType debug level: 0, 1, 2, or 3: ");
DEBUG = atoi(fgets(buff,MAXLINE,stdin));
printf("\nDo you want hard copy? y or n: ");
gets(tch);
if(tolower(tch[0])=='y')
	{
	if ((printer = fopen("prn","w")) == NULL)
		openfail("prn");
	}
else
	{
	if ((printer = fopen("temp.xxx","w")) == NULL)
		openfail("temp.xxx");
	printf("\nWriting output on file temp.xxx");
	}
printf("\nType identifying string.");
fprintf(printer,"%s",gets(temp));
getdate(&today);
fprintf(printer,"\nDATE: %d/%d/%d\n",today.da_mon,today.da_day,today.da_year);
printf("\nType name of file containing control info (CR if none). ");
tgets(buff,stdin);
if(strlen(buff) != 0)
	{
	if((control = fopen(buff,"r")) == NULL)
		openfail(buff);
	fprintf(printer,"Old CRN file %s",buff);
	fscanf(control,"%d%d%d%d%d%d",&ncmin,&ncmax,&ni,&nsc,&nsca,&corrsc);
	for(i=0;i<nsc;i++)
		fscanf(control,"%d",sc+i);	/* getting subclass vals for cols */
	fscanf(control,"%d%d%d",&nimin,&nimax,&last_cond);
	fscanf(control,"%d%d",&ncminl,&ncmaxl);
	fscanf(control,"%d%d%d%d",&cval,&ival,&rtval,&scval);
	fscanf(control,"%d%f%d%d",&cutoff,&sigma,&truncelim,&lowcutoff);
	fscanf(control,"%s%s%s%s",sfname,ifname,msfname,mifname);
	fscanf(control,"%s%s%s%s",spname,ipname,mspname,mipname);
	fscanf(control,"%d%d%d",&ccpl,&icpl,&median);
	fclose(control);
	nc = ncmaxl-ncminl+1;			/* addition from CPM */
	}
else
	{
	printf("\nSmallest condition? ");
	ncmin = atoi(fgets(buff,MAXLINE,stdin));
	printf("Largest condition? ");
	ncmax = atoi(fgets(buff,MAXLINE,stdin));
	printf("Smallest item? ");
	nimin = atoi(fgets(buff,MAXLINE,stdin));
	printf("Largest item? ");
	nimax = atoi(fgets(buff,MAXLINE,stdin));
	printf("How many items (per condition block, if desired)? ");
	ni = atoi(fgets(buff,MAXLINE,stdin));
	printf("Do you want to classify data under the condition number of the previous trial\n(i.e., classify question answers under the cond. of their sentence? y or n: ");
	gets(buff);
	last_cond = (tolower(buff[0]) == 'y') ? 1 : 0;
	if(last_cond)
		{
		printf("\nSmallest condition number for previous trial? ");
		ncminl = atoi(fgets(buff,MAXLINE,stdin));
		printf("\nLargest condition number for previous trial? ");
		ncmaxl = atoi(fgets(buff,MAXLINE,stdin));
		}
	else
		{
		ncminl = ncmin;
		ncmaxl = ncmax;
		}
	nc = ncmaxl-ncminl+1;			/* addition from CPM */
	printf("How many subclassification values? ");
	nsc = atoi(fgets(buff,MAXLINE,stdin));
	printf("How many of these to average over? ");
	nsca = atoi(fgets(buff,MAXLINE,stdin));
	printf("How many 'correct' subclassification values? ");
	corrsc = atoi(fgets(buff,MAXLINE,stdin));
	for(i=0;i<nsc;i++)
		{
		printf("  value of subclassification column %d? ",i+1);
		sc[i] = atoi(fgets(buff,MAXLINE,stdin));
		}
	printf("Which position in data indicates condition #? ");
	cval = atoi(fgets(buff,MAXLINE,stdin));
	printf("Which position in data indicates item #? ");
	ival = atoi(fgets(buff,MAXLINE,stdin));
	printf("Which position in data indicates reaction time? ");
	rtval = atoi(fgets(buff,MAXLINE,stdin));
	printf("Which position in data indicates subclassification? ");
	scval = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is absolute cutoff in msec? ");
	cutoff = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is relative cutoff in S.D. units? ");
	sigma = atof(fgets(buff,MAXLINE,stdin));
	printf("Replace or eliminate long RTs? 0 or 1, respectively: ");
	truncelim = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is lower cutoff in msec? ");
	lowcutoff = atoi(fgets(buff,MAXLINE,stdin));
	printf("Do you want means (= 0) or medians (= 1)? ");
	median = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is output file name for subject totals (RT)? ");
	tgets(sfname,stdin);
	if(strlen(sfname) == 0)
		strcpy(sfname,"subfile.dat");
	printf("How many conditions go on one subject output file line? ");
	ccpl = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is output file name for subject subclassification (RT)? ");
	tgets(msfname,stdin);
	if(strlen(msfname) == 0)
		strcpy(msfname,"msubfile.dat");
	printf("What is output file name for item totals (RT)? ");
	tgets(ifname,stdin);
	if(strlen(ifname) == 0)
		strcpy(ifname,"itmfile.dat");
	printf("How many conditions go on one item output file line? ");
	icpl = atoi(fgets(buff,MAXLINE,stdin));
	printf("What is output file name for item subclassification (RT)? ");
	tgets(mifname,stdin);
	if(strlen(mifname) == 0)
		strcpy(mifname,"mitmfile.dat");
	printf("What is output file name for subject totals (prop)? ");
	tgets(spname,stdin);
	if(strlen(spname) == 0)
		strcpy(spname,"psubfile.dat");
	printf("What is output file name for subject subclassification (prop)? ");
	tgets(mspname,stdin);
	if(strlen(mspname) == 0)
		strcpy(mspname,"mpsbfile.dat");
	printf("What is output file name for item totals (prop)? ");
	tgets(ipname,stdin);
	if(strlen(ipname) == 0)
		strcpy(ipname,"pitmfile.dat");
	printf("What is output file name for item subclassification (prop)? ");
	tgets(mipname,stdin);
	if(strlen(mipname) == 0)
		strcpy(mipname,"mpitmfil.dat");
	}

while(correct(&cutoff,&sigma,&truncelim,sfname,msfname,ifname,mifname,spname,mspname,ipname,mipname,&ccpl,&icpl,&median))	/* allow changes */
	;

nc = ncmaxl-ncminl+1;
printf("\n%d conditions: from %d to %d (labeled as %d to %d)",nc,ncmin,ncmax,ncminl,ncmaxl);
printf("\nWhat file name do you want to save these values as (CR if none)? ");
tgets(file,stdin);
if(strlen(file) != 0)
	{
	if ((control = fopen(file,"w")) == NULL)
		openfail(file);
	fprintf(printer,"CRN file name %s",file);
	fprintf(control,"%d %d %d %d %d %d\n",ncmin,ncmax,ni,nsc,nsca,corrsc);
	for(i=0;i<nsc;i++)
		fprintf(control," %d",sc[i]);	/* getting subclass vals for cols */
	fprintf(control,"\n%d %d %d",nimin,nimax,last_cond);
	fprintf(control,"\n%d %d",ncminl,ncmaxl);
	fprintf(control,"\n%d %d %d %d",cval,ival,rtval,scval);
	fprintf(control,"\n%d %f %d %d",cutoff,sigma,truncelim,lowcutoff);
	fprintf(control,"\n%s %s %s %s",sfname,ifname,msfname,mifname);
	fprintf(control,"\n%s %s %s %s",spname,ipname,mspname,mipname);
	fprintf(control,"\n%d %d %d",ccpl,icpl,median);
	fclose(control);
	}

if ((subfile = fopen(sfname,"w")) == NULL)
	openfail(sfname);
if ((msubfile = fopen(msfname,"w")) == NULL)
	openfail(sfname);
if ((psubfile = fopen(spname,"w")) == NULL)
	openfail(spname);
if ((pmsubfile = fopen(mspname,"w")) == NULL)
	openfail(mspname);

printf("\nFiles opened, now allocating space for arrays.");

if((totmat =(long int *)calloc(nc*nsc,4)) == NULL)
	oops("totmat");
else
	printf("\ntotmat = %p",totmat);
if((nmat = (int *)calloc(nc*nsc,2)) == NULL)
	oops("nmat");
else
	printf("\nnmat = %p",nmat);
if((nrejtot = calloc(nc,2)) == NULL)
	oops("nrejtot");
else
	printf("\nnrejtot = %x",nrejtot);
if((totsum = (long int *)calloc(nc,4)) == NULL)
	oops("totsum");
else
	printf("\ntotsum = %p",totsum);
if((sumsq = (double *)calloc(nc,sizeof(double))) == NULL)
	oops("sumsq");
else
	printf("\nsumsq = %p",sumsq);
if((isumsq = (double *)calloc(nc*ni,sizeof(double))) == NULL)
	oops("isumsq");
else
	printf("\nisumsq = %p",isumsq);
if((nsum = (int *)calloc(nc,2)) == NULL)
	oops("nsum");
else
	printf("\nnsum = %p",nsum);
if((nrejsum = (int *)calloc(nc*ni,2)) == NULL)
	oops("nrejsum");
else
	printf("\nnrejsum = %p",nrejsum);
if((itotmat = (long int *)calloc(ni*nc*nsc,4)) == NULL)
	oops("itotmat");
else
	printf("\nitotmat = %p",itotmat);
if((inmat = (int *)calloc(ni*nc*nsc,2)) == NULL)
	oops("inmat");
else
	printf("\ninmat = %p",inmat);
if((itotsum = (long int *)calloc(ni*nc,4)) == NULL)
	oops("itotsum");
else
	printf("\nitotsum = %p",itotsum);
if((insum = (int *)calloc(ni*nc,2)) == NULL)
	oops("insum");
else
	printf("\ninsum = %p",insum);
if((cntsum = (int *)calloc(nc,2)) == NULL)
	oops("cntsum");
else
	printf("\ncntsum = %p",cntsum);
if((corrsum = (int *)calloc(nc,2)) == NULL)
	oops("corrsum");
else
	printf("\ncorrsum = %p",corrsum);
if((icntsum = (int *)calloc(ni*nc,2)) == NULL)
	oops("icntsum");
else
	printf("\nicntsum = %p",icntsum);
if((icorrsum = (int *)calloc(ni*nc,2)) == NULL)
	oops("icorrsum");
else
	printf("\nicorrsum = %p",icorrsum);
if(median)
	{
	if((mdnrts = (int *)calloc(600,2)) == NULL)
		oops("mdnrts");
	if((items_array = (int *)calloc(ni*nc*200,2)) == NULL)
		oops("items_array");
	if((items_count_array = (int *)calloc(ni*nc,2)) == NULL)
		oops("items_array");
	}

printf
("\nName of file containing data file names? ");
while((control = fopen(gets(file),"r")) == NULL)
	printf("\nBad file name, try again ");
ns = 0;
while((c = fgetc(control)) != EOF)	/* count the subjects */
	{
	printf("%c",c);
	if(c == '\n')
		ns++;
	}

printf("\n\n%d subjects",ns);
printf("\nDo you want printer output of subject means and bad RTs? y or n ");
fgets(buff,MAXLINE,stdin);
spcon = (tolower(buff[0])) == 'y' ? 1 : 0;

printf("\nIs there an exceptions file (format = item# +-condition#) - y or n?");
gets(file);
if(tolower(file[0]) == 'y')
	{
	exceptflag = 1;
	printf("Exceptions file name: ");
	while((efp = fopen(gets(file),"r")) == NULL)
		printf("\nBAD FILE NAME, try again ");
	fprintf(printer,"\nEXCEPTIONS FILE: %s",file);
	while(fscanf(efp,"%d %d",&ei,&ej) != EOF)
		{
		fprintf(printer,"\nItem %d condition adjustment %d",ei,ej);
		vexcept[ei-1] = ej;	/* vector of condition adjustment values */
		}
	}
else
	exceptflag = 0;

rewind(control);
for(i=0;i<ns;i++)
	{
	bdos(11,0,0);		/* check for operator interrupt */
	fscanf(control,"%s",sfile);
	n = filtsub(cutoff,sigma,truncelim,sfile,spcon,exceptflag);
	if(n != ERR)
		sort(n,median,ccpl);
	else
		printf("\nERROR on subject %d, %s",ns+1,sfile);
	}
end:
if (fclose(subfile) == EOF)
	closefail(subfile);
if (fclose(msubfile) == EOF)
	closefail(msubfile);
if (fclose(psubfile) == EOF)
	closefail(psubfile);
if (fclose(pmsubfile) == EOF)
	closefail(pmsubfile);
fclose(control);

if ((subfile = fopen(sfname,"r")) == NULL)
	openfail(sfname);
if ((msubfile = fopen(msfname,"r")) == NULL)
	openfail(sfname);
if ((psubfile = fopen(spname,"r")) == NULL)
	openfail(spname);
if ((pmsubfile = fopen(mspname,"r")) == NULL)
	openfail(mspname);

printf("\nGOING TO WRITEMEANS");
writemeans(ns);

writeitem(ifname,mifname,ipname,mipname,icpl,median);
fclose(printer);
}

/* routine to allow correction of parameter values */
/* revised to include lowcutoff */

int correct(int *cutoff,float *sigma,int *truncelim,char *sfname,char *msfname,
char *ifname,char *mifname,char *spname,char *mspname,char *ipname,
char *mipname,int *ccpl,int *icpl,int *median)
{
int i,j,k;
float fj;
printf("\n Type a number to change a control parameter, or CR to quit.");
printf("\n0: min condition =        %4d",ncmin);
printf("              1: max condition =       %4d",ncmax);
printf("\n2: min item =             %4d",nimin);
printf("              3: max item =            %4d",nimax);
printf("\n4: last_cond =            %4d",last_cond);
printf("\n5: items(/cond block)=    %4d",ni);
printf("\n6: subclassifications =   %4d",nsc);
printf("\n7: subclass to sum =      %4d",nsca);
printf("              8: 'correct' subclass =  %4d",corrsc);
printf("\n9: value of subclass columns = ");
for(i=0;i<nsc;i++)
	printf(" %d",sc[i]);
printf("\n10: position of condition %4d",cval);
printf("             11: position of item      %4d",ival);
printf("\n12: position of RT        %4d",rtval);
printf("             13: position of subclass  %4d",scval);
printf("\n14: cutoff               %5d",*cutoff);
printf("             15: sigma                 %4f",*sigma);
printf("\n16: trunc = 0, elim = 1   %4d",*truncelim);
printf("\n17: lowcutoff             %4d",lowcutoff);
printf("\n18: File: RT sum,subj    %12s",sfname);
printf("     19: Entries per line = %4d",*ccpl);
printf("\n20: File: RT matrix,subj %12s",msfname);
printf("\n21: File: RT sum,item    %12s",ifname);
printf("     22: Entries per line = %4d",*icpl);
printf("\n23: File: RT matrix,item %12s",mifname);
printf("\n24: File: pr sum,subj    %12s",spname);
printf("\n25: File: pr matrix,subj %12s",mspname);
printf("\n26: File: pr sum,item    %12s",ipname);
printf("\n27: File: pr matrix,item %12s",mipname);
if(last_cond)
	{
	printf("\n28: Min cond of previous item: %d",ncminl);
	printf("  29: Max cond of previous item: %d",ncmaxl);
	}
printf("\n30: Mean (0) or Median (1): %d  ---- ? ",*median); 


gets(buff);
if(strlen(buff) == 0)
	{
	if(!last_cond)
		{
		ncminl = ncmin;
		ncmaxl = ncmax;
		}
	fprintf(printer,"\nmin condition =       %4d",ncmin);
	fprintf(printer,"\nmax condition =       %4d",ncmax);
	fprintf(printer,"\nmin item =            %4d",nimin);
	fprintf(printer,"\nmax item =            %4d",nimax);
	fprintf(printer,"\nlast_cond =           %4d",last_cond);
	if(last_cond)
		{
		fprintf(printer,"\nmin cond last item=%4d",ncminl);
		fprintf(printer,"\nmax cond last item=%4d",ncmaxl);
		}
	fprintf(printer,"\nitems(/cond block) =  %4d",ni);
	fprintf(printer,"\nsubclassifications =  %4d",nsc);
	fprintf(printer,"\nsubclass to sum =     %4d",nsca);
	fprintf(printer,"\n'correct' subclass =  %4d",corrsc);
	fprintf(printer,"\nvalue of subclass columns = ");
	for(i=0;i<nsc;i++)
		fprintf(printer," %d",sc[i]);
	fprintf(printer,"\nposition of condition %4d",cval);
	fprintf(printer,"\nposition of item      %4d",ival);
	fprintf(printer,"\nposition of RT        %4d",rtval);
	fprintf(printer,"\nposition of subclass  %4d",scval);
	fprintf(printer,"\ncutoff               %5d",*cutoff);
	fprintf(printer,"\nsigma                 %4f",*sigma);
	fprintf(printer,"\ntrunc = 0, elim = 1   %4d",*truncelim);
	fprintf(printer,"\nlowcutoff =           %4d",lowcutoff);
	fprintf(printer,"\nmean = 0,median = 1   %4d",median);
	fprintf(printer,"\nFile: RT sum,subj     %s",sfname);
	fprintf(printer,"\nEntries per line      %4d",*ccpl);
	fprintf(printer,"\nFile: RT matrix,subj  %s",msfname);
	fprintf(printer,"\nFile: RT sum,item     %s",ifname);
	fprintf(printer,"\nEntries per line      %4d",*icpl);
	fprintf(printer,"\nFile: RT matrix,item  %s",mifname);
	fprintf(printer,"\nFile: pr sum,subj     %s",spname);
	fprintf(printer,"\nFile: pr matrix,subj  %s",mspname);
	fprintf(printer,"\nFile: pr sum,item     %s",ipname);
	fprintf(printer,"\nFile: pr matrix,item  %s  ---  ? ",mipname);
	fprintf(printer,"\n\n\n\n");
	return(NULL);
	}
i = atoi(buff);
if(i == 15)	/* sigma, float */
	{
	printf("\nNew value? ");
	fj = atof(gets(buff));
	*sigma = fj;
	}
else if((i >= 0 && i < 9) || (i > 9&&i < 18) || i==19 || i==22 || i==28 || i==29  || i==30)
	{
	printf("\nNew value? ");
	j = atoi(gets(buff));
	switch(i)
		{
		case 0:
			ncmin = j;
			break;
		case 1:
			ncmax = j;
			nc = ncmax - ncmin + 1;
			break;
		case 2:
			nimin = j;
			break;
		case 3:
			nimax = j;
			break;
		case 4:
			last_cond = j;
		case 5:
			ni = j;
			break;
		case 6:
			nsc = j;
			break;
		case 7:
			nsca = j;
			break;
		case 8:
			corrsc = j;
			break;
		case 10:
			cval = j;
			break;
		case 11:
			ival = j;
			break;
		case 12:
			rtval = j;
			break;
		case 13:
			scval = j;
			break;
		case 14:
			*cutoff = j;
			break;
		case 16:
			*truncelim = j;
			break;
		case 17:
			lowcutoff = j;
			break;
		case 19:
			*ccpl = j;
			break;
		case 22:
			*icpl = j;
			break;
		case 28:
			ncminl = j;
			break;
		case 29:
			ncmaxl = j;
			break;
		case 30:
			*median = j;
			break;
		}
	}
else if (i == 9)
	{
	for (k = 0; k < nsc; k++)
		{
		printf("value of subclassification column %d? ",k+1);
		sc[k] = atoi((gets(buff)));
		}
	}
else if (i > 17 && i < 28)
	{
	printf("File name? ");
	switch(i)
		{
		case 18:
			gets(sfname);
			break;
		case 20:
			gets(msfname);
			break;
		case 21:
			gets(ifname);
			break;
		case 23:
			gets(mifname);
			break;
		case 24:
			gets(spname);
			break;
		case 25:
			gets(mspname);
			break;
		case 26:
			gets(ipname);
			break;
		case 27:
			gets(mipname);
			break;
		}
	}
return(1);	/* ok, normal return */
}

/************************************************************************/
void oops(char *string)
{
printf("\nOut of allocation space at %s.",string);
exit(1);
}



/*************************************************************************/
/* read in subject's data, throw away outliers,
leave in trial structs */


int filtsub(int cutoff,float sigma,int truncelim,char *filename,int spcon,int exceptflag)
{
FILE *subbuff;
int i,j,tsc,nf;
double rts,rtsq,nrt;
extern double sqrt();
int last_cond_value = ncminl;

/*printf("\XXX!!! filename %s.",filename);*/

if((subbuff=(fopen(filename,"r"))) == NULL)
	{
	fprintf(printer,"\n!!!!!!!  Can't open subject file %s.!!!!!!!",filename);
	return(ERR);
	}
fprintf(printer,"\nFile %s.",filename);
/* printf("\nXXX File %s.",filename);*/

/* get data into trials struct */

nf = 0;
while(fgets(buff,MAXLINEX,subbuff) != NULL)
	{
	trials[nf].cond = atoi(buff+spacebuff(cval));
	trials[nf].item = atoi(buff+spacebuff(ival));
	if(exceptflag)	/* exceptions flag set */
		trials[nf].cond += vexcept[(trials[nf].item) -1];	/* correction */
	tsc = atoi(buff+spacebuff(scval));
	trials[nf].subclass = nsc;	/* dummy to discard */

	if(tsc < 0)
		trials[nf].subclass = nsc;	/* KEEP 0 subclass */
	else
	{
	for(j=0;j<nsc;j++)
		if(sc[j]==tsc)
			{
			trials[nf].subclass = j;
			break;
			}
	}
	if(trials[nf].cond >= ncmin && trials[nf].cond <= ncmax && trials[nf].subclass < nsc
		&& trials[nf].item >= nimin && trials[nf].item <= nimax)
		{
		trials[nf].lcond = last_cond_value;	/* copy from previous trial */
		trials[nf].rt = atoi(buff+spacebuff(rtval));
		if (DEBUG)
		printf("\nXXXFilter nc %d nlc %d ni %d sc %d rt %d",trials[nf].cond,trials[nf].lcond,trials[nf].item,trials[nf].subclass,trials[nf].rt);
		nf++;
		}
	else if(trials[nf].cond >= ncminl && trials[nf].cond <= ncmaxl)
		last_cond_value = trials[nf].cond;		/* save it for the next trial */
	}

/* filter out data */
rts = (rtsq = (nrt = 0));	/* initialize */

for (i=0;i<nf;i++)
	{
	if(trials[i].rt < lowcutoff)
		{
		if(spcon)
			fprintf(printer,"\n lowcutoff %d %d %d %d %d",trials[i].cond,trials[i].lcond,trials[i].subclass,trials[i].item,trials[i].rt);
		}
	if(trials[i].rt >= cutoff)
		{
		if(spcon)
			fprintf(printer,"\n cutoff %d %d %d %d %d",trials[i].cond,trials[i].lcond,trials[i].subclass,trials[i].item,trials[i].rt);
		if(truncelim)
			trials[i].rt=0;	/* truncelim = 1, eliminate */
		else
			trials[i].rt=cutoff;	/* truncelim = 0, truncate */
		(*(nrejsum + trials[i].cond - ncmin + (trials[i].item-nimin)*nc))++;
		}
	 if(trials[i].subclass < corrsc && trials[i].rt != 0
			&& trials[i].item >= nimin && trials[i].item <= nimax)
						/* counting from zero */
						/* only paying attention to items within range */
		{
		rts = rts + (double)(trials[i].rt);
		rtsq = rtsq + (double)(trials[i].rt) * (double)(trials[i].rt);
		nrt += 1;
		}
	}

/* calculate variance and CI times variance, as rts */
if(nrt != 0)
	{
	rts /= nrt;
	rtsq = (rtsq/nrt - rts*rts);
	rtsq = sqrt(rtsq);
	if(spcon)
		fprintf(printer,"\n   Mean %g S.D. %g",rts,rtsq);
	rts = rts + (double)(sigma)*rtsq;
	}
else
	{
	rts = 0;
	fprintf(printer,"\n!!! NO RTs SUBJECT %s",filename);
	}
	
for(i=0;i<nf;i++)
	if(trials[i].rt > (int)(rts) && trials[i].subclass <= corrsc && trials[i].rt != cutoff)
		{
		(*(nrejsum + trials[i].cond - ncmin + (trials[i].item-nimin)*nc))++;
		if(spcon)
			fprintf(printer,"\n sigmacutoff %d %d %d %d",trials[i].cond,trials[i].subclass,trials[i].item,trials[i].rt);
		if(truncelim)
			trials[i].rt = 0;
		else
			trials[i].rt = (int)(rts);
		}
fclose(subbuff);
return(nf);
}


/***************************************************************************/
/* get specified datum from buff */
spacebuff(val)
int val;
{
int i,j;
val--;
j=0;

if(val EQ 0)
	{
	while(isspace(buff[j]))
		j++;
	return(j);
	}

for(i=0;i<val;i++)
	{
	while(isspace(buff[j]))
		j++;	/* move to first/next nonspace */
	while(!isspace(buff[j]))
		j++;	/* then move to next space */
	while(isspace(buff[j]))
		j++;	/* and finally on to next nonspace */
	}
	j--;
return(j);
}



/*********************************************************************/
/* sort the filtered data into the allocated regions */


void sort(int nf,int median,int ccpl)
{
double tempmean;
int i,j,k,mptr,cptr,ip;
int tnc,tmax,tmin,tcond;
int last_cond_value;
int imptr,icptr,jccpl;
int adj_item;
k = nc*nsc;
if(!median)
{
for(i=0;i<k;i++)
	{
	*(totmat+i) = 0l;
	*(nmat+i) = 0;
	}
for(i=0;i<nc;i++)
	{
	*(totsum+i) = 0l;
	*(sumsq+i)=0;
	*(nsum+i) = 0;
	*(cntsum+i) = 0;
	*(corrsum+i) = 0;
	}
for(i=0;i<nf;i++)
	{
	if(last_cond)
		{
	 	mptr = (trials[i].lcond - ncminl) * nsc + trials[i].subclass;
		cptr = trials[i].lcond - ncminl;
		if(cptr < 0 | cptr >= nc)
			{
			printf("\nOOPS: Cond (%d) outside range of ncminl (%d) - ncmaxl (%d), trial %d",trials[i].lcond,ncminl,ncmaxl,i+1);
			exit(1);
			}
		}
	else
		{
		mptr = (trials[i].cond - ncmin) * nsc + trials[i].subclass;
		cptr = trials[i].cond - ncmin;
		}
	adj_item = ((trials[i].item - nimin)%ni);
	imptr = (adj_item) * nc * nsc + mptr;
	icptr = (adj_item) * nc + cptr;
	*(cntsum+cptr) += 1;
	*(icntsum+icptr) += 1;
	if(trials[i].subclass < corrsc)
		{
		*(corrsum+cptr) += 1;
		*(icorrsum+icptr) += 1;
		}
	if(trials[i].rt > lowcutoff)
		{
		*(totmat+mptr) += (long)(trials[i].rt);
		*(nmat+mptr) += 1;
		*(itotmat + imptr) += (long)(trials[i].rt);
		*(inmat + imptr) += 1;
		if(trials[i].subclass < nsca)
			{
			*(totsum + cptr) += (long)(trials[i].rt);
			*(sumsq + cptr) += (double)(trials[i].rt) * (double)(trials[i].rt);
			*(nsum + cptr) += 1;
			*(itotsum + icptr) += (long)(trials[i].rt);
			*(isumsq + icptr) += (double)(trials[i].rt) * (double)(trials[i].rt);
			*(insum + icptr) += 1;
			}
		if (DEBUG == 2)
		{
		printf("\nXXXSort, sn %d in %d totmat %ld itotmat %ld",*(nmat+mptr),*(inmat+imptr),*(totmat+mptr),*(itotmat+imptr));
		printf("\nXXX   snsum %d insum %d totsum %ld itotsum %ld ",*(nsum+cptr),*(insum+icptr),*(totsum+cptr),*(itotsum+icptr));
		printf("\nXXX  addresses: totsum+cptr, %x, nsum+cptr, %x",&(*(totsum+cptr)),&(*(nsum+cptr)));
		printf("\nXXX  imptr %d icptr %d mptr %d cptr %d",imptr,icptr,mptr,cptr);
		}
		}
	else
		{
		if(DEBUG == 2)
			printf("\nXXXSort fastresp, imptr %d icptr %d mptr %d cptr %d",imptr,icptr,mptr,cptr);
		}
	}
	for(i=0,jccpl=1;i<nc;i++,jccpl++)
		{
		cptr = i;
/*		if(*(nsum+cptr) != 0)
			fprintf(subfile,"%6ld",*(totsum+cptr)/(long)(*(nsum+cptr)));
		else
			fprintf(subfile,"%6d",0);*/
		
		if(*(nsum+cptr) != 0)
			{
			tempmean = (double)(*(totsum+cptr)/(long)(*(nsum+cptr)));
			fprintf(subfile,"%10.0lf",*(sumsq+cptr)/(double)(*(nsum+cptr)) - tempmean * tempmean);
			}
		else
			fprintf(subfile,"%10d",0);
		
		if(*(cntsum+cptr) != 0)
			fprintf(psubfile,"%6.3f",(double)(*(corrsum+cptr))/(double)(*(cntsum+cptr)));
		else
			fprintf(psubfile,"%6.3f",0);
		for(j=0;j<nsc;j++)
			{
			mptr = (i * nsc) + j;
			if(*(nmat+mptr) != 0)
				fprintf(msubfile,"%6ld",*(totmat+mptr)/(long)(*(nmat+mptr)));
			else
				fprintf(msubfile,"%6d",0);
			fprintf(pmsubfile,"%6d",*(nmat+mptr));
			}
		fprintf(msubfile,"\n");
		fprintf(pmsubfile,"\n");
		if(jccpl == ccpl || i == nc-1)
			{
			jccpl = 0;
			fprintf(subfile,"\n");
			fprintf(psubfile,"\n");
			}
		}
	}
else	/* medians */
	{
	if(last_cond)
		{
		tmin = ncminl;
		tmax = ncmaxl;
		}
	else
		{
		tmin = ncmin;
		tmax = ncmax;
		}
	for(tnc=tmin;tnc<=tmax;tnc++)
		{
		for(i=0,ip=0;i<nf;i++)		
			{
			if(last_cond)
				tcond = trials[i].lcond;
			else
				tcond = trials[i].cond;
			if(tcond == tnc)
				{
if(DEBUG)
	printf("\nMDN match cond = %d trial = %d observation %d",tnc,i,ip);
				if(last_cond)
					{
				 	mptr = (trials[i].lcond - ncminl) * nsc + trials[i].subclass;
					cptr = trials[i].lcond - ncminl;
					if(cptr < 0 | cptr >= nc)
						{
						printf("\nOOPS: Cond (%d) outside range of ncminl (%d) - ncmaxl (%d), trial %d",trials[i].lcond,ncminl,ncmaxl,i+1);
						exit(1);
						}
					}
				else
					{
					mptr = (trials[i].cond - ncmin) * nsc + trials[i].subclass;
					cptr = trials[i].cond - ncmin;
					}
				adj_item = ((trials[i].item - nimin)%ni);
				imptr = (adj_item) * nc * nsc + mptr;
				icptr = (adj_item) * nc + cptr;
				*(cntsum+cptr) += 1;
				*(icntsum+icptr) += 1;
				if(trials[i].subclass < corrsc)
					{
					*(corrsum+cptr) += 1;
					*(icorrsum+icptr) += 1;
					}
				if(trials[i].rt > lowcutoff)
					{
					*(nmat+mptr) += 1;
					*(inmat + imptr) += 1;
					if(trials[i].subclass < nsca)
						{
						mdnrts[ip++] = trials[i].rt;
						*(nsum + cptr) += 1;
						*(insum + icptr) += 1;
						/* store data for item */
						*(items_array+(icptr*200 + *(items_count_array+icptr))) = trials[i].rt;
						(*(items_count_array + icptr))++;
						}
					if (DEBUG == 2)
						{
						printf("\nXXXSort, sn %d in %d totmat %ld itotmat %ld",*(nmat+mptr),*(inmat+imptr),*(totmat+mptr),*(itotmat+imptr));
						printf("\nXXX   snsum %d insum %d totsum %ld itotsum %ld ",*(nsum+cptr),*(insum+icptr),*(totsum+cptr),*(itotsum+icptr));
						printf("\nXXX  addresses: totsum+cptr, %x, nsum+cptr, %x",&(*(totsum+cptr)),&(*(nsum+cptr)));
						printf("\nXXX  imptr %d icptr %d mptr %d cptr %d",imptr,icptr,mptr,cptr);
						}
					}
				else
					{
					if(DEBUG == 2)
						printf("\nXXXSort fastresp, imptr %d icptr %d mptr %d cptr %d",imptr,icptr,mptr,cptr);
					}
				}		
			}			/* end of this condition */
			if(ip != 0)	/* get the median, add it in */
			{
			if(DEBUG)
				{
				printf("\nMDN CALCULATION, %d scores, cptr = %d",ip,cptr);
				printf("\n  initial mdnrts[ip/2] = %d",mdnrts[ip/2]);
				}
			qsort(mdnrts,ip,sizeof(mdnrts[0]),order);	/* perform sort */
			*(totsum + cptr) = (long)((mdnrts[(ip-1)/2] + mdnrts[(ip)/2])/2);
			if(DEBUG)
				{
				printf("\n  final mdnrts[ip/2] = %d",mdnrts[(ip)/2]);
				}
			*(itotsum + icptr) += *(totsum + cptr);
			}		
		}
	for(i=0,jccpl=1;i<nc;i++,jccpl++)
		{
		cptr = i;
		if(*(nsum+cptr) != 0)
			fprintf(subfile,"%6ld",*(totsum+cptr));
		else
			fprintf(subfile,"%6d",0);
		if(*(cntsum+cptr) != 0)
			fprintf(psubfile,"%6.3f",(double)(*(corrsum+cptr))/(double)(*(cntsum+cptr)));
		else
			fprintf(psubfile,"%6.3f",0);
		for(j=0;j<nsc;j++)
			{
			mptr = (i * nsc) + j;
			if(*(nmat+mptr) != 0)
				fprintf(msubfile,"%6ld",*(totmat+mptr));
			else
				fprintf(msubfile,"%6d",0);
			fprintf(pmsubfile,"%6d",*(nmat+mptr));
			}
		fprintf(msubfile,"\n");
		fprintf(pmsubfile,"\n");
		if(jccpl == ccpl || i == nc-1)
			{
			jccpl = 0;
			fprintf(subfile,"\n");
			fprintf(psubfile,"\n");
			}
		}
	}
}



		

/************************************************************************/

writeitem(ifname,mifname,ipname,mipname,icpl,median)
char *ifname,*mifname,*ipname,*mipname,icpl,median;
{
double tempmean;
int i,j,k,jicpl,icount;
FILE *itemfile,*mitemfile;
FILE *pitemfile,*pmitemfile;
double temp;
int cptr,iptr;
printf("\nDo you want item by item data files written? y or n: ");
fgets(buff,MAXLINE,stdin);
if(tolower(buff[0]) == 'n')
	return;

if ((itemfile = fopen(ifname,"w")) == NULL)
	openfail(ifname);
if ((mitemfile = fopen(mifname,"w")) == NULL)
	openfail(mifname);
if ((pitemfile = fopen(ipname,"w")) == NULL)
	openfail(ipname);
if ((pmitemfile = fopen(mipname,"w")) == NULL)
	openfail(mipname);

printf("\nDo you want item by item printer output? y or n: ");
fgets(buff,MAXLINE,stdin);			/* leave the answer in buff[0] */

for (i=0; i < ni; i++)
	{
	bdos(11,0,0);
	if(buff[0] == 'y')
		fprintf(printer,"\nItem %4d ",i+1);
	for(j=0,jicpl=1;j < nc;j++,jicpl++)
		{
		if(!median)
			{
			cptr = i*nc + j;
			if(*(insum+cptr) != 0)
				{
				tempmean = (double)(*(itotsum + cptr))/(long)(*(insum + cptr));
				fprintf(itemfile,"%10.0lf",(temp = *(isumsq+cptr)/(double)(*(insum+cptr)) - tempmean*tempmean));
				}
			else
				fprintf(itemfile,"%10.0lf",(temp = 0));
			if(*(icntsum+cptr) != 0)
				fprintf(pitemfile,"%6.3f",(double)(*(icorrsum+cptr))/(double)(*(icntsum+cptr)));
			else
				fprintf(pitemfile,"000000");
			if(buff[0] == 'y')
				fprintf(printer,"\n   Cond %4d TOT %6ld SC",j+1,temp);
			for(k=0;k<nsc;k++)
				{
				iptr = i*nc*nsc + j*nsc + k;
				if(*(inmat + iptr) != 0)
					fprintf(mitemfile,"%6ld",(temp = (*(itotmat + iptr)/(long)(*(inmat + iptr)))));
				else
					fprintf(mitemfile,"%6ld",(temp = 0));
				fprintf(pmitemfile,"%6d",*(inmat+iptr));
				if(buff[0] == 'y')
					fprintf(printer,"%6ld",temp);
				}
			fprintf(mitemfile,"\n");
			fprintf(pmitemfile,"\n");
			if(jicpl == icpl || j == nc-1)
				{
				jicpl = 0;
				fprintf(itemfile,"\n");
				fprintf(pitemfile,"\n");
				}
			}
		else			/* median */
			{
			cptr = i*nc + j;
			icount = *(items_count_array + cptr);
			qsort(items_array+(cptr*200),icount,sizeof(items_array[0]),order);	/* perform sort */
			temp = (*(items_array+(cptr*200+(icount-1)/2)) + *(items_array+(cptr*200+icount/2)))/2;
			fprintf(itemfile,"%6d",temp);
			if(DEBUG)
				{
				printf("\n items median %d, cond %d, item %d mdnrts[ip/2] = %d",temp,j,i);
				}
			if(*(icntsum+cptr) != 0)
				fprintf(pitemfile,"%6.3f",(double)(*(icorrsum+cptr))/(double)(*(icntsum+cptr)));
			else
				fprintf(pitemfile,"000000");
			if(buff[0] == 'y')
				fprintf(printer,"\n   Cond %4d TOT %6ld SC",j+1,temp);
			if(jicpl == icpl || j == nc-1)
				{
				jicpl = 0;
				fprintf(itemfile,"\n");
				fprintf(pitemfile,"\n");
				}
			}
		}
	}
fprintf(printer,"\f");
if (fclose(itemfile) == EOF)
	closefail(itemfile);
if (fclose(mitemfile) == EOF)
	closefail(mitemfile);
if (fclose(pitemfile) == EOF)
	closefail(pitemfile);
if (fclose(pmitemfile) == EOF)
	closefail(pmitemfile);
return;
}



int order(first,second)
int *first;
int *second;
{
if(*first < *second)
	return(-1);
else if(*first == *second)
	return(0);
else
	return(1);
}





void writemeans(int ns)
{
int i,j,k,mptr;
double tempr;
float tempp;
int tempi;
rewind(subfile);
rewind(msubfile);
rewind(psubfile);
rewind(pmsubfile);

/* do it all first for RT means */

for(i=0;i<nc;i++)
	{
	for(j=0;j<nsc;j++)
		{
		mptr = i*nsc + j;
		*(totmat+mptr) = 0l;
		*(nmat+mptr) = 0;
		}
	*(nsum+i) = 0;
	*(totsum+i) = 0l;
	}

for(i=0;i<ns;i++)
	{
	for(j=0;j<nc;j++)
		{
		for(k=0;k<nsc;k++)
			{
			mptr = j*nsc + k;
			fscanf(msubfile,"%lf",&tempr);
			if (DEBUG == 3)
				printf("\n  --tempr mat %ld i %d j %d k %d",tempr,i,j,k);
			if(tempr > 0)
				{
				*(nmat+mptr) += 1;
				*(totmat+mptr) += tempr;
				}
			}
		fscanf(subfile,"%lf",&tempr);
		if (DEBUG == 3)
			printf("\n   --tempr tot %lf i %d j %d",tempr,i,j);
		if(tempr > 0)
			{
			*(nsum + j) += 1;
			*(sumsq + j) += tempr;
			if (DEBUG == 3)
				printf("        cum %lf n %d",*(sumsq+j),*(nsum + j));
			}
		}
	}
fprintf(printer,"\nMEANS OF INDIVIDUAL SUBJECT VARIANCES \n");
for(i=0;i<nc;i++)
	{
	if(*(nsum + i) != 0)
		fprintf(printer,"\nCond %3d RT Total %10.0lf     SC",i+ncminl,*(sumsq+i)/(double)(*(nsum+i)));
	else
		fprintf(printer,"\nCond %3d RT Total  000000000     SC",i+ncminl);
	for(j=0;j<nsc;j++)
		{
		mptr = i*nsc + j;
		if(*(nmat+mptr) != 0)
			fprintf(printer,"%6ld",*(totmat+mptr)/(long)*(nmat+mptr));
		else
			fprintf(printer," 00000");
		}
	}

/* and again for mean proportions */

fprintf(printer,"\n\n\n");

for(i=0;i<nc;i++)
	{
	for(j=0;j<nsc;j++)
		{
		mptr = i*nsc + j;
		*(totmat+mptr) = 0;
		*(nmat+mptr) = 0;
		}
	*(nsum+i) = 0;
	*(totsum+i) = 0;
	}

for(i=0;i<ns;i++)
	{
	for(j=0;j<nc;j++)
		{
		for(k=0;k<nsc;k++)
			{
			mptr = j*nsc + k;
			fscanf(pmsubfile,"%d",&tempi);
			*(totmat+mptr) += (long)(tempi);
			}
		fscanf(psubfile,"%f",&tempp);
		*(totsum + j) += (long)(tempp*1000);
		*(nsum + j) += 1;
		if(DEBUG == 3)
			printf("\n      cond j %d *(totsum+j) %ld *(nsum+j) %d",j,*(totsum+j),*(nsum+j));
		}
	}

for(i=0;i<nc;i++)
	{
	fprintf(printer,"\nCond %3d PC Total %6.3f     SC",i+ncminl,((double)(*(totsum+i)/(long)(*(nsum+i))))/1000);
	for(j=0;j<nsc;j++)
		{
		mptr = i*nsc + j;
		fprintf(printer,"%6ld",*(totmat+mptr));
		}
	}

/* print matrix of number rejected */


fprintf(printer,"\n\n\nREJECTS: Item x cond");
fprintf(printer,"\nITEM                  COND\n");  
fprintf(printer,"\n   ");
for(j=0;j<nc;j++)
	{
	fprintf(printer,"%3d",j+ncmin);
	nrejtot[j] = 0;
	}
fprintf(printer,"\n\n");
for(i=0;i<ni;i++)
	{
	fprintf(printer,"\n%3d",i+1);
	for(j=0;j<nc;j++)
		{
		nrejtot[j] += (*(nrejsum + i*nc + j));
		fprintf(printer,"%3d",(*(nrejsum + i*nc +j)));
		}
	}
fprintf(printer,"\n\nTOT");
for(j=0;j<nc;j++)
	fprintf(printer,"%3d",nrejtot[j]);

fprintf(printer,"\f");
}

void openfail(char *filestr)
{
printf("\nCan't open file %s. ",filestr);
exit(1);
}

closefail(filestr)
FILE *filestr;
{
printf("\nCan't close file %d; check disk space! ",filestr);
return;
}


/*rewind(stream)
FILE *stream;
{
fseek(stream,0L,0);
}*/

char *tgets(buff,stream)
char *buff;
FILE *stream;
	{
	int i;
	fgets(buff,MAXLINE,stream);
	for(i=0;buff[i] != '\n' && i < MAXLINE;i++)
		;
	buff[i] = '\0';
	return(buff);
	}
