#include <stdio.h>
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

void __attribute__((constructor)) init(){
	suspiciousCerts = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
}

bool mySecCertificateIsValid(SecCertificateRefP certificate, CFAbsoluteTime verifyTime)
{
	if(CFSetContainsValue(suspiciousCerts, certificate))
	{
		return 0; //hax
	}
	return SecCertificateIsValid(certificate, verifyTime);
}

/**
called by SecTrustServerEvaluateAsync to copy the certificate chain and the anchors
anchors are trusted certificates provided by the caller (if any), we should not care about them but heh
**/
CFArrayRef mySecCertificateDataArrayCopyArray(CFArrayRef dataarray)
{
	CFArrayRef res = SecCertificateDataArrayCopyArray(dataarray);
	int l = CFArrayGetCount(res);
	int i=0;
	for(i=0 ; i < l; i++)
	{
		if(i != 0) //ignore the leaf certificate
		{
			SecCertificateRefP cert = (SecCertificateRefP) CFArrayGetValueAtIndex(res,i);
			if(cert == NULL)
				continue;
			const SecCEBasicConstraints* constraints = SecCertificateGetBasicConstraints(cert);
			if(constraints == NULL)
				continue;
			
			if(!constraints->isCA)	//if not leaf and not isCA then bad boy
			{
				if(!CFSetContainsValue(suspiciousCerts, cert))
				{
					CFSetAddValue(suspiciousCerts, cert);
					fprintf(stderr, "SSLFix rejecting certificate\n");
					CFStringRef desc = SecCertificateCopySubjectSummary(cert);
					if(desc != NULL)
					{
						CFShow(desc);
						CFRelease(desc);
					}
				}
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
