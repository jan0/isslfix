iOS < 4.3.5 fix for SSL vulnerability (CVE-2011-0228)
=====================================================

Visit https://issl.recurity.com to check if this is working.
If you already visited this page without the fix applied, reload the page or clear Safari's cache.
You should see the "Cannot Verify Server Identity" popup, and this message in syslog :

```
<Warning>: iSSLFix: Certificate <1BDC0A9E-7FC6-4BA4-A9E5-41F206B82D81> in chain starting at <issl.recurity.com> has isCA=0 => possible MITM attempt, making validation fail
```

If for some reason securityd crashes, do not reboot and remove the package (dpkg -r isslfix).

Deb package : https://github.com/downloads/jan0/isslfix/isslfix-test2.deb

References
==========

http://support.apple.com/kb/HT4824
http://blog.recurity-labs.com/archives/2011/07/26/cve-2011-0228_ios_certificate_chain_validation_issue_in_handling_of_x_509_certificates/
http://blog.spiderlabs.com/2011/07/twsl2011-007-ios-ssl-implementation-does-not-validate-certificate-chain.html
http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2011-0228

