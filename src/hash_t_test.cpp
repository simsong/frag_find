
#include "hash_t.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

int main(int argc,char **argv)
{
    u_char buf[64];
    memset(buf,0,sizeof(buf));

    md5_t null_md5 = md5_generator::hash_buf(buf,0);
    sha1_t null_sha1 = sha1_generator::hash_buf(buf,0);
    sha256_t null_sha256 = sha256_generator::hash_buf(buf,0);

    std::cout << "hashing the null block:\n";
    std::cout << "md5:    " << null_md5.hexdigest() << "\n";
    assert(null_md5 == md5_t::fromhex("d41d8cd98f00b204e9800998ecf8427e"));

    std::cout << "sha1:   " << null_sha1.hexdigest() << "\n";
    assert(null_sha1 == sha1_t::fromhex("da39a3ee5e6b4b0d3255bfef95601890afd80709"));

    std::cout << "sha256: " << null_sha256.hexdigest() << "\n";
    assert(null_sha256 == sha256_t::fromhex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

#ifdef HAVE_SHA512_T
    sha512_t null_sha512 = sha512_generator(buf,0);
    std::cout << "sha512: " << null_sha512.hexdigest() << "\n";
#endif

    /* pre-initialize a hash from values */
    md5_t md50(buf);
    std::cout << "md50:  " << md50.hexdigest() << "\n";
    md5_t md51 = md5_t::fromhex("00000000000000000000000000000000");
    
    assert(md50 == md51);

    return(0);
}
