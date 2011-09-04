#include <stdio.h>
#include <syslog.h>
#include <CoreFoundation/CoreFoundation.h>
#include "comodo.h"
#include "diginotar.h"

#define SecCertificateRefP void*	//moo
CFDataRef SecCertificateGetNormalizedIssuerContent(SecCertificateRefP);
CFDataRef SecCertificateCopySerialNumber(SecCertificateRefP);

/**
 * Add check for blacklisted Comodo certificates for iOS < 4.3.2
 * http://support.apple.com/kb/HT4606
 * http://www.comodo.com/Comodo-Fraud-Incident-2011-03-23.html
 * https://bugzilla.mozilla.org/show_bug.cgi?id=643056
 * http://hg.mozilla.org/mozilla-central/rev/f6215eef2276

 * iOS 4.3.2 introducted a new kSecPolicyCheckBlackListedLeaf policy
 * that checks the issuer name and serial number against hardcoded values
**/

int check_comodo_blacklist(SecCertificateRefP cert)
{
    int i;
  
    //check if issuer is UTNB
    CFDataRef issuer = SecCertificateGetNormalizedIssuerContent(cert);

    //CFDataGetLength() == 0x97
    if (issuer == NULL || CFDataGetLength(issuer) != sizeof(comodo_utnb_issuer))
        return 0;

    if(memcmp(CFDataGetBytePtr(issuer), comodo_utnb_issuer, sizeof(comodo_utnb_issuer)))
        return 0;

    CFDataRef serial = SecCertificateCopySerialNumber(cert);
    if (serial == NULL) //should not happen but whatever
        return 0;

    int sl = CFDataGetLength(serial);
    const UInt8* p = CFDataGetBytePtr(serial);
    
    if( p == NULL)
        return 0

    //skip leading null bytes 
    while(sl > 0 && *p == 0)
    {
        p++;
        sl--;
    }
    
    if (sl == SERIALSIZE)
    {
        for(i=0; i < N_COMODO_SERIALS; i++)
        {
            //XXX apple only memcmp 4 bytes ? false positives ?
            if(!memcmp(p, comodo_serials[i], SERIALSIZE))
            {
                syslog(LOG_WARNING, "iSSLFix: blocking blacklisted Comodo certificate");
                CFRelease(serial);
                return 1;
            }
        }
    }

    CFRelease(serial);
    return 0;
}

/**
 * Blacklist DigiNotar roots
 * http://codereview.chromium.org/7791032/
 * https://bugzilla.mozilla.org/show_bug.cgi?id=682956
**/
int check_diginotar_blacklist(SecCertificateRefP cert)
{
    int i;
    
    CFDataRef pkh_data = SecCertificateCopyPublicKeySHA1Digest(cert);
    if( pkh_data == NULL)
        return 0;
        
    const UInt8* pkh = CFDataGetBytePtr(pkh_data);
    
    if( pkh == NULL)
        return 0;
    
    for(i=0; i < NUM_DIGINOTAR_PKHS; i++)
    {
        if(!memcmp(pkh, diginotar_pkhs[i], PKH_SIZE))
        {
            syslog(LOG_WARNING, "iSSLFix: blocking DigiNotar certificate");
            CFRelease(pkh_data);
            return 1;
        }
    }
    CFRelease(pkh_data);

    return 0;
}