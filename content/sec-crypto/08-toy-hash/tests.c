int main(void){const uint8_t m[][4]={{0},{'a'},{'a','b'},{255,0,128,1}};size_t ns[]={0,1,2,4};unsigned wants[]={197,44,202,237};
 for(size_t i=0;i<4;i++)CHECK("tiny hash vector",tiny_hash(m[i],ns[i])==wants[i],"vector %zu",i);
 uint16_t a=0,b=0;int rc=first_collision(&a,&b);CHECK("first collision",rc==1 && a==239 && b==256,"got %u and %u",a,b);
 uint8_t x[2]={(uint8_t)a,(uint8_t)(a>>8)},y[2]={(uint8_t)b,(uint8_t)(b>>8)};
 CHECK("distinct equal hashes",a!=b && tiny_hash(x,2)==tiny_hash(y,2),"");
 CHECK("NULL outputs",first_collision(NULL,&b)==0 && first_collision(&a,NULL)==0,"");return 0;}
