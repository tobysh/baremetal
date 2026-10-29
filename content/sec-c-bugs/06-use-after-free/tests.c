int main(void){const char *s[]={"","a","longer label","percent %n and spaces"};
 for(size_t i=0;i<4;i++){char src[100];strcpy(src,s[i]);char *a=owned_label(src),*b=owned_label(src);int ok=a && b && a!=b && a!=src && b!=src && !strcmp(a,s[i]) && !strcmp(b,s[i]);
 src[0]='X';if(a && strcmp(a,s[i]))ok=0;if(a && a[0]){a[0]='!';if(b && strcmp(b,s[i]))ok=0;}
 CHECK("independent live owners",ok,"input %zu",i);free(a);free(b);}
 CHECK("NULL has no owner",owned_label(NULL)==NULL,"");return 0;}
