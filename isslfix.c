#include <stdio.h>
#include <syslog.h>
#include <CoreFoundation/CoreFoundation.h>

/**
Fix for iOS SSL fail
http://support.apple.com/kb/HT4824
http://blog.recurity-labs.com/archives/2011/07/26/cve-2011-0228_ios_certificate_chain_validation_issue_in_handling_of_x_509_certificates/
http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2011-0228

This fix is a hack, replicating the official patch is complicated 
**/

//http://www.opensource.apple.com/source/libsecurity_keychain/libsecurity_keychain-55029/lib/certextensionsP.h
typedef struct {
	bool				present;
	bool				critical;
	bool				isCA;
	bool				pathLenConstraintPresent;
	uint32_t			pathLenConstraint;
} SecCEBasicConstraints;

#define SecCertificateRefP void*	//moo

const SecCEBasicConstraints* SecCertificateGetBasicConstraints(SecCertificateRefP);
bool SecCertificateIsValid(SecCertificateRefP certificate, CFAbsoluteTime verifyTime);
CFStringRef SecCertificateCopySubjectSummary(SecCertificateRefP);
CFArrayRef SecCertificateDataArrayCopyArray(CFArrayRef);

CFMutableSetRef suspiciousCerts = NULL;

//this symbol was introduced in iOS 4.3.2
__attribute__((weak_import)) extern void* kSecPolicyCheckBlackListedLeaf;


void __attribute__((constructor)) init(){
	suspiciousCerts = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
}

bool mySecCertificateIsValid(SecCertificateRefP certificate, CFAbsoluteTime verifyTime)
{
	if(suspiciousCerts != NULL && CFSetContainsValue(suspiciousCerts, certificate))
	{
		return 0; //hax
	}
    //if < 4.3.2 then do our own check for Comodo certs
    if(&kSecPolicyCheckBlackListedLeaf == NULL && isCertificateBlackListed(certificate))
    {
        return 0; //hax
    }
	return SecCertificateIsValid(certificate, verifyTime);
}

char buf1[256]={0};
char buf2[256]={0};
/**
called by SecTrustServerEvaluateAsync to copy the certificate chain and the anchors
anchors are trusted certificates provided by the caller (if any), we should not care about them but heh
**/
CFArrayRef mySecCertificateDataArrayCopyArray(CFArrayRef dataarray)
{
	CFArrayRef res = SecCertificateDataArrayCopyArray(dataarray);
	int l = CFArrayGetCount(res);
	if(l == 0)
		return res;
	int i=0;

	//http://developer.apple.com/library/mac/documentation/Security/Reference/certifkeytrustservices/Reference/reference.html#//apple_ref/c/func/SecTrustCreateWithCertificates
	//"The certificate to be verified must be the first in the array"
	SecCertificateRefP leaf = (SecCertificateRefP) CFArrayGetValueAtIndex(res, 0);
	
	for(i=1 ; i < l; i++)
	{
			SecCertificateRefP cert = (SecCertificateRefP) CFArrayGetValueAtIndex(res,i);
			if(cert == NULL)
				continue;
			
			if(leaf != NULL && CFEqual(leaf, cert)) //ignore when leaf cert appears multiple times
				continue;
			
			const SecCEBasicConstraints* constraints = SecCertificateGetBasicConstraints(cert);
			if(constraints == NULL)
				continue;
			
			if(!constraints->isCA)	//if not leaf and not isCA then bad boy
			{
				if(!CFSetContainsValue(suspiciousCerts, cert))
				{
					CFSetAddValue(suspiciousCerts, cert);
					CFStringRef desc = SecCertificateCopySubjectSummary(cert);
					CFStringRef desc_leaf = SecCertificateCopySubjectSummary(leaf);
					if(desc != NULL)
					{
						CFStringGetCString(desc, buf1, 255, kCFStringEncodingASCII);
						CFRelease(desc);
					}
					if(desc_leaf != NULL)
					{
						CFStringGetCString(desc_leaf, buf2, 255, kCFStringEncodingASCII);
						CFRelease(desc_leaf);
					}
					syslog(LOG_WARNING, "iSSLFix: Certificate <%s> in chain starting at <%s> has isCA=0 => possible MITM attempt, making validation fail", buf1, buf2);
				}
			}
	}
	return res;
}

//disable securityd idle timer for easy debugging
/*void myCFRunLoopAddTimer(){
	return;
}*/

const struct {void* n; void* o;} interposers[] __attribute((section("__DATA, __interpose"))) = 
{
	//{ (void*) myCFRunLoopAddTimer, (void*) CFRunLoopAddTimer }, //uncomment this for debugging
	{ (void*) mySecCertificateDataArrayCopyArray, (void*) SecCertificateDataArrayCopyArray },
	{ (void*) mySecCertificateIsValid, (void*) SecCertificateIsValid }
};
