int main(void){
 const size_t a[][2]={{0,0},{0,SIZE_MAX},{3,4},{SIZE_MAX,0},{SIZE_MAX,1},{SIZE_MAX/2,2},{SIZE_MAX/2+1,2},{1,SIZE_MAX},{1,SIZE_MAX-1},{SIZE_MAX,8}};
 for(size_t i=0;i<sizeof a/sizeof a[0];i++){size_t c=a[i][0],w=a[i][1],out=77;int want=!w || c<=(SIZE_MAX-1)/w;int rc=allocation_size(c,w,&out);
 CHECK("size arithmetic",rc==want && out==(want?c*w+1:77),"case %zu",i);}
 CHECK("NULL output",allocation_size(1,1,NULL)==0,"");return 0;
}
