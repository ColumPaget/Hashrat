#include "UnitsOfMeasure.h"

double ToPower(double val, double power)
{
    double result=0;
    int i;

    result=val;
    for (i=1; i < power; i++)
    {
        result=result * val;
    }

    return(result);
}


double FromSIUnit(const char *Data, int Base)
{
    double val;
    char *ptr=NULL;

    val=strtod(Data,&ptr);
    while (isspace(*ptr)) ptr++;
    switch (*ptr)
    {
    case 'k':
        val=val * Base;
        break;
    case 'M':
        val=val * ToPower(Base,2);
        break;
    case 'G':
        val=val * ToPower(Base,3);
        break;
    case 'T':
        val=val * ToPower(Base,4);
        break;
    case 'P':
        val=val * ToPower(Base,5);
        break;
    case 'E':
        val=val * ToPower(Base,6);
        break;
    case 'Z':
        val=val * ToPower(Base,7);
        break;
    case 'Y':
        val=val * ToPower(Base,8);
        break;
    }

    return(val);
}



const char *ToSIUnit(double Value, int Base, int Precision)
{
    static char *Str=NULL;
    char *Fmt=NULL;
    double next;
//Set to 0 to keep valgrind happy
    int i=0;
    char suffix=' ', *sufflist=" kMGTPEZY";


    for (i=0; sufflist[i] !='\0'; i++)
    {
        next=ToPower(Base, i+1);
        if (next > Value) break;
    }

    if ((i > 0) && (sufflist[i] !='\0'))
    {
        Value=Value / ToPower(Base, i);
        suffix=sufflist[i];
        Fmt=FormatStr(Fmt, "%%0.%df%%c", Precision);
        Str=FormatStr(Str,Fmt,(float) Value,suffix);
    }
    else
    {
        //here 'next' is the remainder, by casting 'Value' to a long we remove the
        //decimal component, then subtract from Value. This leaves us with *only*
        //the decimal places
        next=Value - (long) Value;
        if (Precision==0) Str=FormatStr(Str,"%ld",(long) Value);
        else
        {
            Fmt=FormatStr(Fmt, "%%0.%df", Precision);
            Str=FormatStr(Str,Fmt,(float) Value);
        }
    }


    DestroyString(Fmt);
    return(Str);
}

