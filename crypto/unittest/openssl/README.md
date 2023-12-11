# certificates for validation

All certificates generated using openssl command line utility.

## private key

3072 bit RSA private key:
```
$ openssl rsa -text -noout -in rsapriv.pem 
$ openssl rsa -text -noout -inform DER -in rsapriv.crt
Private-Key: (3072 bit, 2 primes)
modulus:
    00:ab:f8:10:fe:60:a3:6c:20:da:ca:a1:70:1a:63:
```

## public key

RSA public key corresponding to the private key:
```
$ openssl rsa -RSAPublicKey_in -in rsapub.pem -noout -text
$ openssl rsa -RSAPublicKey_in -inform DER -in rsapub.crt -noout -text
Public-Key: (3072 bit)
Modulus:
    00:bd:13:f1:d1:e9:77:e5:0e:d0:2d:c7:03:7a:ed:
```

## X.509 cerificate

X.509 self-signed certificate using the private key:
```
$ openssl x509 -text -noout -in rsacert.pem
$ openssl x509 -text -noout -inform DER -in rsacert.crt
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            47:30:77:bd:b3:2b:8b:93:f7:cd:68:50:d4:26:d6:23:61:36:d1:bf
        Signature Algorithm: sha512WithRSAEncryption
        Issuer: C = US, ST = CA, L = San Jose, O = "Stable Cloud Computing, Inc.", CN = stablecc.com
        Validity
            Not Before: Dec 10 21:08:32 2023 GMT
            Not After : Dec  9 21:08:32 2024 GMT
        Subject: C = US, ST = CA, L = San Jose, O = "Stable Cloud Computing, Inc.", CN = stablecc.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (3072 bit)
                Modulus:
                    00:bd:13:f1:d1:e9:77:e5:0e:d0:2d:c7:03:7a:ed:
```

## alternative formats

Openssl RSA public key format:
```
$ openssl rsa -pubin -in osslpub.pem -noout -text
```

RSA SSH2 public key export in RFC 4716 format:
```
$ cat sshpub.pem 
---- BEGIN SSH2 PUBLIC KEY ----
Comment: "3072-bit RSA, converted by mike@mike-3530 from OpenSSH"
AAAAB3NzaC1yc2EAAAADAQABAAABgQC9E/HR6XflDtAtxwN67RSfXTV6JQ6CIWCHVfX4/i
```

SSH RSA public key `.ssh/id_rsa.pub` format:
```
$ cat rsapub.pem 
-----BEGIN RSA PUBLIC KEY-----
MIIBigKCAYEAvRPx0el35Q7QLccDeu0Un101eiUOgiFgh1X1+P4o0r6w71KJl1ET
```

Elliptical curve cryptography ECDSA:
```
$ openssl ecparam -text -noout -in ecpriv.pem # 521 bit private key
EC-Parameters: (521 bit)
ASN1 OID: secp521r1
NIST CURVE: P-521
$ openssl ec -pubin -in ecpub.pem -noout -text # 521 bit public key
read EC key
Public-Key: (521 bit)
pub:
    04:01:d1:6b:94:d6:58:79:12:86:87:6e:c9:af:56:
$ openssl x509 -text -noout -in eccert.pem # self-signed X.509 certificate signed with EC 512
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            0e:98:f4:f7:be:5e:b9:b3:38:7f:93:83:8e:4b:f3:a9:7d:5c:53:00
        Signature Algorithm: ecdsa-with-SHA512
        Issuer: C = US, ST = CA, L = San Jose, O = "Stable Cloud Computing, Inc.", CN = stablecc.com
        Validity
            Not Before: Dec 10 22:21:16 2023 GMT
            Not After : Dec  9 22:21:16 2024 GMT
        Subject: C = US, ST = CA, L = San Jose, O = "Stable Cloud Computing, Inc.", CN = stablecc.com
        Subject Public Key Info:
            Public Key Algorithm: id-ecPublicKey
                Public-Key: (521 bit)
                pub:
                    04:01:d1:6b:94:d6:58:79:12:86:87:6e:c9:af:56:
```
