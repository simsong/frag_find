
#include "hash_t.h"

int main(int argc,char **argv)
{
    u_char buf[64];
    memset(buf,0,sizeof(buf));

    md5_t md0(buf);
    sha1_t sha0(buf);

    buf[3] = 0x33;

    md5_t md1(buf);
    sha1_t sha1(buf);
    const sha1_t *sha2 = sha1_t::new_from_hex("0000003300000000000000000000000000000000");

    printf("sizeof(sha0)=%zd  %s\n",sizeof(sha0),sha0.hexdigest().c_str());
    printf("sizeof(sha1)=%zd  %s\n",sizeof(sha1),sha1.hexdigest().c_str());
    printf("sizeof(sha2)=%zd  %s\n",sizeof(sha2),sha2->hexdigest().c_str());

    printf("sha0==sha1? %d\n",sha0==sha1);
    printf("sha1==sha12 %d\n",sha1==*sha2);
}
