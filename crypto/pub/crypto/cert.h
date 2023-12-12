/*
BSD 3-Clause License

Copyright (c) 2022, Stable Cloud Computing, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _SCC_CRYPTO_CERT_H
#define _SCC_CRYPTO_CERT_H

#include <map>
#include <memory>
#include <set>
#include <system_error>
#include <fstream>
#include <algorithm>
#include <crypto/der.h>
#include <crypto/rsa.h>
#include <crypto/ecc.h>
#include <crypto/bignum.h>

namespace scc::crypto {

/** \addtogroup crypto
	@{
*/

/** \defgroup crypto_cert X.509 and RSA certificates
	@{

	Certificate encoding using PEM format: https://tools.ietf.org/html/rfc7468

	RSA public and private key ASN.1 syntax: https://tools.ietf.org/html/rfc8017#appendix-A
	PKCS#1: https://tools.ietf.org/html/rfc8017#page-68

	X.509 certificates: https://tools.ietf.org/html/rfc5280
	Information on algorithms used in certificates: https://tools.ietf.org/html/rfc3279
	PKCS exchange syntax: https://tools.ietf.org/html/rfc7292
	Rsa syntax defined in: https://tools.ietf.org/html/rfc3447#page-44
*/

/** X.509 and RSA certificates.
	\file
*/

/** Key algorithm type.

	rsa public keys have algorithm_id = {1, 2, 840, 113549, 1, 1, 1}, see https://tools.ietf.org/html/rfc3279#section-2.3.1
	ecdsa public keys have algorithm_id = {1, 2, 840, 10045, 2, 1}, see https://tools.ietf.org/html/rfc3279#section-2.3.5

	ecdsa supports standard curves where the parameters are an oid identifying the named (standard) curve,
	see https://tools.ietf.org/html/rfc3279#page-19, and https://tools.ietf.org/html/rfc5480#page-17
*/
enum KeyAlgoType
{
	unknown = 0,
	rsa,			///< parameter null
	ec_p192r1,		///< parameter {1, 2, 840, 10045, 3, 1, 1}
	ec_p224r1,		///< parameter {1, 3, 132, 0, 33}
	ec_p256r1,		///< parameter {1, 2, 840, 10045, 3, 1, 7}
	ec_p384r1,		///< parameter {1, 3, 132, 0, 34}
	ec_p521r1,		///< parameter {1, 3, 132, 0, 35}
};

/**	Public key information certificate.

	Defined in X.509.

	SubjectPublicKeyInfo  ::=  SEQUENCE  {
		algorithm            AlgorithmIdentifier,
		subjectPublicKey     BIT STRING -- DER encoded ASN.1 public key
	}

	AlgorithmIdentifier  ::=  SEQUENCE  {
        algorithm               OBJECT IDENTIFIER,
        parameters              ANY DEFINED BY algorithm OPTIONAL
	}

	PEM header is BEGIN PUBLIC KEY (https://tools.ietf.org/html/rfc7468).
*/
struct PublicKeyCert
{
	PublicKeyCert() {}
	virtual ~PublicKeyCert() {}

	oid_value algorithm_id;				///< Algorithm id
	BasePtr parameters;					///< The optional parameters (may be null)
	std::vector<uint8_t> public_key;	///< The uninterpreted public key

	/** Return the embedded public key type. */
	KeyAlgoType type() const;

	/** Parse from a sequence. */
	void parse(const BasePtr);
	void parse(const DerDocument& doc)
	{
		parse(doc.root_ptr());
	}

	/** Dump to a sequence. */
	BasePtr dump() const;

	/** Print descriptive string. */
	std::string str() const;

	/** Get rsa public key. Throws exception if not an RSA algo, or an invalid key. */
	void get(RsaPublicKey& key) const;
	/** Set rsa public key. */
	void set(const RsaPublicKey& key);

	/** Get ecdsa public key. Throws exception if not an EC algo, or an invalid point. */
	void get(EccGfpPoint&) const;
	/** Set ecdsa public key and algorithm. Throws exception if not an EC algo type, or an invalid point. */
	void set(const KeyAlgoType&, const EccGfpPoint&);
};

/** RSA public key certificate.

	From https://tools.ietf.org/html/rfc2437#section-11.1.1
	
	RSAPublicKey ::= SEQUENCE {
		modulus           INTEGER,  -- n
		publicExponent    INTEGER   -- e
	}

	PEM header is BEGIN RSA PUBLIC KEY.
*/
struct RsaPublicKeyCert
{
	/** Parse from a sequence. Throws exception on error. */
	static void parse(const BasePtr&, RsaPublicKey&);
	static void parse(const DerDocument& doc, RsaPublicKey& key)
	{
		parse(doc.root_ptr(), key);
	}

	/** Dump to a sequence. */
	static BasePtr dump(const RsaPublicKey&);
};

/** RSA private key certificate.

	From https://tools.ietf.org/html/rfc2437#section-11.1.2

	RSAPrivateKey ::= SEQUENCE {
		version           Version, -- 0, meaning no otherPrimeInfos
		modulus           INTEGER,  -- n
		publicExponent    INTEGER,  -- e
		privateExponent   INTEGER,  -- d
		prime1            INTEGER,  -- p
		prime2            INTEGER,  -- q
		exponent1         INTEGER,  -- d mod (p-1)  -- ep
		exponent2         INTEGER,  -- d mod (q-1)  -- eq
		coefficient       INTEGER,  -- (inverse of q) mod p  -- qinv
		otherPrimeInfos   OtherPrimeInfos OPTIONAL (not for version 0)
	}
	
	PEM header is BEGIN RSA PRIVATE KEY.
*/
struct RsaPrivateKeyCert
{
	/** Parse from a sequence. Throws exception on error. */
	static void parse(const BasePtr&, RsaPrivateKey&);
	static void parse(const DerDocument& doc, RsaPrivateKey& key)
	{
		parse(doc.root_ptr(), key);
	}

	/** Dump to a sequence. */
	static BasePtr dump(const RsaPrivateKey&);
};

/** From: https://tools.ietf.org/html/rfc3279#section-2.3.5

	EcpkParameters ::= CHOICE {
		ecParameters  ECParameters,
		namedCurve    OBJECT IDENTIFIER,
		implicitlyCA  NULL }

	This implementation supports only named curves.

	PEM certficate format:
	-----BEGIN EC PARAMETERS-----
	<namedCurve>
*/
struct EcParametersCert
{
	/** Parse from an object id.
		Throws an exception if not an object id.
		Sets the key algorithm to an ecdsa or unknown type.
	*/
	static void parse(const BasePtr&, KeyAlgoType&);
	static void parse(const DerDocument& doc, KeyAlgoType& alg)
	{
		parse(doc.root_ptr(), alg);
	}
	
	/** Dump to an object id. Key algorithm must be an ecdsa type.
	*/
	static BasePtr dump(const KeyAlgoType&);
};

/** Elliptic curve public key.

	From: https://tools.ietf.org/html/rfc5480#section-2.2

	The elliptic curve public key (a value of type ECPoint that is
	an OCTET STRING) is mapped to a subjectPublicKey (a value of
	type BIT STRING) as follows: the most significant bit of the
	OCTET STRING value becomes the most significant bit of the BIT
	STRING value, and so on; the least significant bit of the OCTET
	STRING becomes the least significant bit of the BIT STRING.
	Conversion routines are found in Sections 2.3.1 and 2.3.2 of
	[SEC1].

	The first octet of the OCTET STRING indicates whether the key is
	compressed or uncompressed.  The uncompressed form is indicated
	by 0x04 and the compressed form is indicated by either 0x02 or
	0x03 (see 2.3.3 in [SEC1]).  The public key MUST be rejected if
	any other value is included in the first octet.

	This implementation assumes that the key is uncompressed.
*/
struct EcPublicKeyCert
{
	/** Parse from a bit string for a specific curve.
		
		Throws exception if string is compressed, or is an invalid point for the curve.

		The KeyAlgoType type must be one of the ecdsa types.
	*/
	static void parse(const BasePtr&, const KeyAlgoType&, EccGfpPoint&);
	static void parse(const DerDocument& doc, const KeyAlgoType& alg, EccGfpPoint& p)
	{
		parse(doc.root_ptr(), alg, p);
	}
	/** Parse from memory. */
	static void parse(const void*, int, const KeyAlgoType&, EccGfpPoint&);
	static void parse(const std::vector<char>& v, const KeyAlgoType& a, EccGfpPoint& p)
	{
		parse(v.data(), v.size(), a, p);
	}
	static void parse(const std::vector<uint8_t>& v, const KeyAlgoType& a, EccGfpPoint& p)
	{
		parse(v.data(), v.size(), a, p);
	}

	/** Dump to an uncompressed bit string. */
	static BasePtr dump(const EccGfpPoint&);
	/** Dump to memory. Dumps the point directly to the vector. Vector is resized. */
	static void dump(const EccGfpPoint&, std::vector<char>&);
	static void dump(const EccGfpPoint&, std::vector<uint8_t>&);
};

/**	Private key certificate utility.

	From: https://tools.ietf.org/html/rfc5915

	ECPrivateKey ::= SEQUENCE {
		version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
		privateKey     OCTET STRING,
		parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
		publicKey  [1] BIT STRING OPTIONAL
	}

	Note parameters and publickey are recommended, so this implementation will require them.

	privateKey is an octet string of length ceiling (log2(n)/8) (where n is the order of the curve)

	PEM certificate format:
	-----BEGIN EC PRIVATE KEY-----
	<ECPrivateKey>
*/
struct EcPrivateKeyCert
{
	/** Parse from a sequence. Throws exception on error. */
	static void parse(const BasePtr&, Bignum&, KeyAlgoType&, EccGfpPoint&);
	static void parse(const DerDocument& doc, Bignum& priv, KeyAlgoType& alg, EccGfpPoint& pub)
	{
		parse(doc.root_ptr(), priv, alg, pub);
	}

	/** Dump to a sequence. */
	static BasePtr dump(const Bignum&, const KeyAlgoType&, const EccGfpPoint&);
};

/** An x.509 directory string is used to store generic names. Only printable and
	utf8 are recommended in compliant x.509 v3, but other types are supported for backwards
	compatibility. ia5 is used in some root certificates.

	https://tools.ietf.org/html/rfc5280#section-4.1.2.4

	DirectoryString ::= CHOICE {
		teletexString           TeletexString (SIZE (1..MAX)),
		printableString         PrintableString (SIZE (1..MAX)),
		universalString         UniversalString (SIZE (1..MAX)),
		utf8String              UTF8String (SIZE (1..MAX)),
		bmpString               BMPString (SIZE (1..MAX)) }

	ia5 and visible are also supported.
*/
struct DirectoryString : public std::string
{
	enum class Type
	{
		printable,
		utf8,
		universal,
		bmp,
		teletex,
		ia5,
		visible,
	};

	DirectoryString::Type type;

	DirectoryString() : type(DirectoryString::Type::printable) {}
	DirectoryString(const std::string& b, DirectoryString::Type t = Type::printable) : type(t)
	{
		assign(b);
	}
	DirectoryString(const BasePtr& b)
	{
		parse(b);
	}
	virtual ~DirectoryString() {}

	/** Compare two directory strings.
	
		Strict comparison requires conversion to utf-16 and normalization: https://tools.ietf.org/html/rfc4518
	*/
	int compare(const DirectoryString& b) const
	{
		return std::string::compare(b);
	}

	bool operator==(const DirectoryString& b) const { return compare(b) == 0; }
	bool operator!=(const DirectoryString& b) const { return compare(b) != 0; }

	/** Parse the string from a base object. Throws exception if the object is not a string. */
	void parse(BasePtr);

	/** Descriptive string <chars> <type>. */
	std::string str() const;

	BasePtr dump() const;
};

/** Attribute types.

	These are defined in x.509 spec, and various other docs.

	Main https://tools.ietf.org/html/rfc5280#page-110
	
	id-at OBJECT IDENTIFIER ::= { joint-iso-ccitt(2) ds(5) 4 }

*/
enum class AttributeType
{
	unknown,
	name,						///< {2, 5, 4, 41} },
	surname,					///< {2, 5, 4, 4} },
	given_name,					///< {2, 5, 4, 42} },
	generation_qualifier,		///< {2, 5, 4, 44} },
	common_name,				///< {2, 5, 4, 3} },
	locality_name,				///< {2, 5, 4, 7} },
	state_or_province_name,		///< {2, 5, 4, 8} },
	organization_name,			///< {2, 5, 4, 10} },
	organizational_unit_name,	///< {2, 5, 4, 11} },
	title,						///< {2, 5, 4, 12} },
	dn_qualifier,				///< {2, 5, 4, 46} },
	country_name,				///< {2, 5, 4, 6} },
	serial_number,				///< {2, 5, 4, 5} },
	pseudonym,					///< {2, 5, 4, 65} },
	organization_id,			///< {2, 5, 4, 97} },
	street_address,				///< {2, 5, 4, 9} },
	domain_component,			///< { 0, 9, 2342, 19200300, 100, 1, 25 } },
	email_address,				///< {1, 2, 840, 113549, 1, 9, 1} },
};

/** An x.509 relative distingushed name is a set of attribute / directory string names.

	https://tools.ietf.org/html/rfc5280#section-4.1.2.4

	Name ::= CHOICE { -- only one possibility for now --
		rdnSequence  RDNSequence }

	RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

	RelativeDistinguishedName ::=
		SET SIZE (1..MAX) OF AttributeTypeAndValue

	AttributeTypeAndValue ::= SEQUENCE {
		type     AttributeType,
		value    AttributeValue }

	AttributeType ::= OBJECT IDENTIFIER

	AttributeValue ::= ANY -- DEFINED BY AttributeType

*/

using RDNPair = std::pair<oid_value, DirectoryString>;

struct RDNComp
{
	bool operator()(const RDNPair& lhs, const RDNPair& rhs) const
	{
		return lhs.first < rhs.first;
	}
};

struct RelativeDistinguishedName : public std::set<RDNPair, RDNComp>
{
	RelativeDistinguishedName() {}
	RelativeDistinguishedName(BasePtr b)
	{
		parse(b);
	}
	virtual ~RelativeDistinguishedName() {}

	/** Parse from RelativeDistinguishedName. */
	void parse(BasePtr);
	/** Dump and return an element of type set. */
	BasePtr dump() const;
	/** Print contents. */
	std::string str() const;

	static AttributeType type(const oid_value&);

	static const oid_value name;
	static const oid_value surname; 
	static const oid_value given_name; 
	static const oid_value generation_qualifier; 
	static const oid_value common_name; 
	static const oid_value locality_name; 
	static const oid_value state_or_province_name; 
	static const oid_value organization_name; 
	static const oid_value organizational_unit_name; 
	static const oid_value title; 
	static const oid_value dn_qualifier; 
	static const oid_value country_name; 
	static const oid_value serial_number; 
	static const oid_value pseudonym; 
	static const oid_value organization_id; 
	static const oid_value street_address; 
	static const oid_value domain_component; 
	static const oid_value email_address; 
};

/** General name. https://tools.ietf.org/html/rfc5280#page-37.

	A general name can have a variety of types. The default is the directory name type.

	GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName

	GeneralName ::= CHOICE {
		otherName                       [0]     OtherName,
		rfc822Name                      [1]     IA5String,
		dNSName                         [2]     IA5String,
		x400Address                     [3]     ORAddress,
		directoryName                   [4]     Name,
		ediPartyName                    [5]     EDIPartyName,
		uniformResourceIdentifier       [6]     IA5String,
		iPAddress                       [7]     OCTET STRING,
		registeredID                    [8]     OBJECT IDENTIFIER }
		
	OtherName ::= SEQUENCE {
		type-id    OBJECT IDENTIFIER,
		value      [0] EXPLICIT ANY DEFINED BY type-id }

	EDIPartyName ::= SEQUENCE {
		nameAssigner            [0]     DirectoryString OPTIONAL,
		partyName               [1]     DirectoryString }

	ORAddress ::= SEQUENCE {
		built-in-standard-attributes BuiltInStandardAttributes,
		built-in-domain-defined-attributes
						BuiltInDomainDefinedAttributes OPTIONAL,
		-- see also teletex-domain-defined-attributes
		extension-attributes ExtensionAttributes OPTIONAL }

	For convenience, string, Name, and OID types are parsed and dumped into the associated value field string_val, name_val or oid_val.

	Other types can be manipulated through the base field and are not interpreted.
*/
struct GeneralName
{
	enum class Type : int
	{
		other_name = 0,						// base_val
		rfc822_name = 1,					// string_val
		dns_name = 2,						// string_val
		x400_address = 3,					// base_val
		directory_name = 4,					// name_val
		edi_party_name = 5,					// base_val
		uniform_resource_identifier = 6,	// string_val
		ip_address = 7,						// string_val
		registered_id = 8,					// oid_val
	};

	Type type;								///< Value type.

	/** Create the base sequence. For other_name, x400_address, edi_party_name, an empty base sequence is created. */
	GeneralName(Type t = Type::directory_name);
	virtual ~GeneralName() {}

	BasePtr base;
	std::string string_val;
	std::vector<RelativeDistinguishedName> name_val;
	oid_value oid_val;

	void clear()
	{
		string_val.clear();
		name_val.clear();
		oid_val.clear();
		base.reset();
	}

	bool compare(const GeneralName& b) const
	{
		if (type != b.type)		return false;

		switch (type)
		{
		case Type::registered_id:
			return oid_val == b.oid_val;
		case Type::rfc822_name:
		case Type::dns_name:
		case Type::uniform_resource_identifier:
		case Type::ip_address:
			return string_val.compare(b.string_val) == 0;
		case Type::directory_name:
			return name_val == b.name_val;
		case Type::other_name:
		case Type::x400_address:
		case Type::edi_party_name:
			return false;		// not possible to compare raw elements
		}
		return false;
	}

	bool operator==(const GeneralName& b) const { return compare(b); }
	bool operator!=(const GeneralName& b) const { return !compare(b); }

	/** Parse an input as an implicit element (must have id corresponding to the type above. Sets the base pointer to the implicit value. */
	void parse(BasePtr);
	/** Reset the base pointer and dump the element to a context-class element. */
	BasePtr dump();
	/** Print contents. Debug flag also prints out base element contents. */
	std::string str(bool = false) const;
};

enum class ExtType
{
	subject_alternative_name,				// oid = {2, 5, 29, 17}
	authority_key_identifier,				// oid = {2, 5, 29, 35}
	subject_key_identifier,					// oid = {2, 5, 29, 14}
	issuer_alternative_name,				// oid = {2, 5, 29, 18}
	basic_constraints,						// oid = {2, 5, 29, 19}
	key_usage,								// oid = {2, 5, 29, 15}
	extended_key_usage,						// oid = {2, 5, 29, 37}
/*
	Not implemented:

	certificate_policies,					// oid = {2, 5, 29, 32}
	policy_mappings,						// oid = {2, 5, 29, 33}
	subject_directory_attributes,			// oid = {2, 5, 29, 9}
	name_constraints,						// oid = {2, 5, 29, 30}
	policy_constraints,						// oid = {2, 5, 29, 36}
	crl_distribution_points,				// oid = {2, 5, 29, 31}
	inhibit_any_policy,						// oid = {2, 5, 29, 54}
	freshest_crl,							// oid = {2, 5, 29, 46}
	authority_information_access,			// oid = {1, 3, 6, 1, 5, 5, 7, 1, 1}
	subject_information_access,				// oid = {1, 3, 6, 1, 5, 5, 7, 1, 11}
	signed_certificate_timestamp_list,		// oid = {1, 3, 6, 1, 4, 1, 11129, 2, 4, 2}
*/
};

class ExtBase;
using ExtBasePtr = std::shared_ptr<ExtBase>;

/** X.509 extensions.

	From https://tools.ietf.org/html/rfc5280#section-4.1

	Extension  ::=  SEQUENCE  {
		extnID      OBJECT IDENTIFIER,
		critical    BOOLEAN DEFAULT FALSE,
		extnValue   OCTET STRING
					-- contains the DER encoding of an ASN.1 value
					-- corresponding to the extension type identified
					-- by extnID
		}
*/
struct ExtBase
{	
	oid_value oid;				///< The oid of the extension.
	bool critical;				///< Is the extension marked critical? If a CRL contains a critical extension that cannot be processed, it must not be used to determine the status of certificates.
	BasePtr value;				///< Parsed extension value.

	ExtBase(bool crit = false) : critical(crit) {}
	virtual ~ExtBase() {}

	/** Return the name of the extension. */
	virtual std::string name() const { return "ExtUnknown"; }
	/** Print to string, optionally printing the value. */
	virtual std::string str(bool = false) const;
	/** Parse value into the the local sub-class data. */
	virtual void parse() {}
	/** Dump sub-class data into the value. */
	virtual void dump() {}
	/** Is this implemented (sub-classed)?
	
		If an extension is marked critical, but is unrecognized, it is not recommended for the service to proceed using the certificate.
	*/
	virtual bool implemented() const {	return false; }

	/** Create an extension.
		\param seq Extension sequence. Throws exception if this sequence is not of the Extension type.
		Parses the extension sequence, calls parse() for the sub-class, and returns sub-class pointer.
	*/
	static ExtBasePtr create(BasePtr);
	
	/**	Dump the extension into an Extension sequence.
	*/
	BasePtr dump_seq();

	/** Find the oid associated with the extension type. */
	static oid_value find_oid(ExtType);
};

/** Subject alternative name.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.6
	
	SubjectAltName ::= GeneralNames

	GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName	
*/
struct ExtSubjectAlternativeName : public ExtBase
{
	ExtSubjectAlternativeName(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::subject_alternative_name);
	}
	virtual ~ExtSubjectAlternativeName() {}

	std::string name() const { return "ExtSubjectAlternativeName"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtSubjectAlternativeName);
	}
	static ExtSubjectAlternativeName& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtSubjectAlternativeName: invalid cast");
		return *dynamic_cast<ExtSubjectAlternativeName*>(b.get());
	}

	std::vector<GeneralName> names;				///< Alternative names.
};

/** Authority key identifier.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.1

	AuthorityKeyIdentifier ::= SEQUENCE {
		keyIdentifier             [0] KeyIdentifier           OPTIONAL,
		authorityCertIssuer       [1] GeneralNames            OPTIONAL,
		authorityCertSerialNumber [2] CertificateSerialNumber OPTIONAL  }

	KeyIdentifier ::= OCTET STRING
	CertificateSerialNumber  ::=  INTEGER
*/
struct ExtAuthorityKeyIdentifier : public ExtBase
{
	ExtAuthorityKeyIdentifier(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::authority_key_identifier);
	}
	virtual ~ExtAuthorityKeyIdentifier() {}

	std::string name() const { return "ExtAuthorityKeyIdentifier"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtAuthorityKeyIdentifier);
	}
	static ExtAuthorityKeyIdentifier& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtAuthorityKeyIdentifier: invalid cast");
		return *dynamic_cast<ExtAuthorityKeyIdentifier*>(b.get());
	}

	std::vector<char> key_identifier;					///< Identifies the private key used to sign.
	std::vector<GeneralName> authority_cert_issuer;		///< Certificate issuer.
	Bignum authority_cert_serial_number;				///< Serial number of the certificate.
};


/** Subject key identifier.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.2

	SubjectKeyIdentifier ::= KeyIdentifier
*/
struct ExtSubjectKeyIdentifier : public ExtBase
{
	ExtSubjectKeyIdentifier(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::subject_key_identifier);
	}
	virtual ~ExtSubjectKeyIdentifier() {}

	std::string name() const { return "ExtSubjectKeyIdentifier"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtSubjectKeyIdentifier);
	}
	static ExtSubjectKeyIdentifier& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtSubjectKeyIdentifier: invalid cast");
		return *dynamic_cast<ExtSubjectKeyIdentifier*>(b.get());
	}

	std::vector<char> key_identifier;
};

/** Issuer alternative name.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.7

	IssuerAltName ::= GeneralNames (sequence of GeneralName)
*/
struct ExtIssuerAlternativeName : public ExtBase
{
	ExtIssuerAlternativeName(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::issuer_alternative_name);
	}
	virtual ~ExtIssuerAlternativeName() {}

	std::string name() const { return "ExtIssuerAlternativeName"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtIssuerAlternativeName);
	}
	static ExtIssuerAlternativeName& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtIssuerAlternativeName: invalid cast");
		return *dynamic_cast<ExtIssuerAlternativeName*>(b.get());
	}

	std::vector<GeneralName> names;
};

/** Basic constraints.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.9

	BasicConstraints ::= SEQUENCE {
		cA                      BOOLEAN DEFAULT FALSE,
		pathLenConstraint       INTEGER (0..MAX) OPTIONAL }

	CA certificates with public keys used to validate certificates must have this extension, and it must be critical.
*/
struct ExtBasicConstraints : public ExtBase
{
	ExtBasicConstraints(bool crit=false, bool ca=false, unsigned plen=0) : ExtBase(crit), conditional_access(ca), max_path_len(plen)
	{
		oid = ExtBase::find_oid(ExtType::basic_constraints);
	}
	virtual ~ExtBasicConstraints() {}

	std::string name() const { return "ExtBasicConstraints"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtBasicConstraints);
	}
	static ExtBasicConstraints& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtBasicConstraints: invalid cast");
		return *dynamic_cast<ExtBasicConstraints*>(b.get());
	}

	bool conditional_access;				///< Is this a conditional access certificate? If so, the public key can be used to verify certificate.
	Bignum max_path_len;					///< If conditional access, the maximum number of intermediate certificates in the certification path.
};

/** Key usage. https://tools.ietf.org/html/rfc5280#section-4.2.1.3

	KeyUsage ::= BIT STRING {
			digitalSignature        (0),
			nonRepudiation          (1), -- recent editions of X.509 have
								-- renamed this bit to contentCommitment
			keyEncipherment         (2),
			dataEncipherment        (3),
			keyAgreement            (4),
			keyCertSign             (5),
			cRLSign                 (6),
			encipherOnly            (7),
			decipherOnly            (8) }
*/
struct ExtKeyUsage : public ExtBase
{
	ExtKeyUsage(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::key_usage);
		clear();
	}
	virtual ~ExtKeyUsage() {}

	std::string name() const { return "ExtKeyUsage"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtKeyUsage);
	}
	static ExtKeyUsage& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtKeyUsage: invalid cast");
		return *dynamic_cast<ExtKeyUsage*>(b.get());
	}

	void clear()
	{
		digital_signature = false;
		content_commitment = false;
		key_encipherment = false;
		data_encipherment = false;
		key_agreement = false;
		key_cert_sign = false;
		crl_sign = false;
		encipher_only = false;
		decipher_only = false;
	}

	bool digital_signature;		///< Public key is used for verifying digital signatures other than certificates and CRLs.
	bool content_commitment;	///< Public key is used for verifying digital signatures in a content commitment (non-repudiation) service.
	bool key_encipherment;		///< Public key is used to encipher private keys, e.g. in key transport.
	bool data_encipherment;		///< Public key is used to encipher data. This should be rare, as most applications will use key transport to establish a symmetric key.
	bool key_agreement;			///< Public key is used for key agreement, e.g. Diffie-Hellman key management.
	bool key_cert_sign;			///< Public key is used for verifying signatures on public key certificates. ExtBasicConstraints conditional_access must be true if this is true.
	bool crl_sign;				///< Public key is used for verifying certificates on certificate revocation lists, e.g. CRLs.
	bool encipher_only;			///< If key_agreement set, public key can only be used for enciphering data while performing key agreement.
	bool decipher_only;			///< If key_agreement set, public key can only be used for deciphering data while performing key agreement.
};

/** Extended key usage.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.12

	ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId

	KeyPurposeId ::= OBJECT IDENTIFIER

	Extended key usage must be consistent with key usage.
*/
struct ExtExtendedKeyUsage : public ExtBase
{
	ExtExtendedKeyUsage(bool crit=false) : ExtBase(crit)
	{
		oid = ExtBase::find_oid(ExtType::extended_key_usage);
		clear();
	}
	virtual ~ExtExtendedKeyUsage() {}

	std::string name() const { return "ExtExtendedKeyUsage"; }
	virtual std::string str(bool = false) const;
	virtual void parse();
	virtual void dump();
	virtual bool implemented() const {	return true; }

	static bool is_castable(ExtBasePtr b)
	{
		return typeid(*b) == typeid(ExtExtendedKeyUsage);
	}
	static ExtExtendedKeyUsage& cast(ExtBasePtr b)
	{
		if (b == nullptr || !is_castable(b)) 	throw std::runtime_error("ExtExtendedKeyUsage: invalid cast");
		return *dynamic_cast<ExtExtendedKeyUsage*>(b.get());
	}

	void clear()
	{
		server_auth = false;
		client_auth = false;
		code_signing = false;
		email_protection = false;
		time_stamping = false;
		ocsp_signing = false;
		additional_usage_ids.clear();
	}

	bool permit_any;		///< Permit any usage. Used for applications that must include this extension, but do not wish to specify usages.
	bool server_auth;		///< TLS WWW server auth. Consistent with digital_signature, key_encipherment, key_agreement
	bool client_auth;		///< TLS WWW client auth. Consistent with digital_signature, key_agreement
	bool code_signing;		///< Signing of downloadable code. Consistent with digital_signature.
	bool email_protection;	///< Email protection. Consistent with digital_signature, content_commitment, key_encipherment, key_agreement
	bool time_stamping;		///< Binding the hash of an object to a time. Consistent with digital_signature, content_commitment
	bool ocsp_signing;		///< Signing OCSP responses. Consistent with digital_signature, content_commitment

	std::vector<oid_value> additional_usage_ids;		///< Key usage ids not in the list above.
};

#if 0

/** Signed certificate timestamp list. Used in TLS. https://tools.ietf.org/html/rfc6962#section-3.3.

	The signed certificate timestamp is defined in: https://tools.ietf.org/html/rfc6962#page-13

	TLS encoding of basic structures: https://tools.ietf.org/html/rfc5246#section-4
 */
class ExtSignedCertificateTimestampList : public ExtBase
{
	static int struct_size(const void*, unsigned int);
	bool add_sct(const void*, int, std::string&);
	int dump_scts(std::vector<char>&);
public:
	std::string name() const { return "ExtSignedCertificateTimestampList"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	std::vector<char> val;

	enum class HashAlgorithm : int
	{
		none = 0,
		md5,
		sha1,
		sha224,
		sha256,
		sha384,
		sha512,
	};

	enum class SigAlgorithm : int
	{
		anonymous = 0,
		rsa,
		dsa,
		ecdsa,
	};

	struct SCT
	{
		std::vector<char> log_id;		///< Log id
		uint64_t timestamp;				///< Milliseconds since epoch
		HashAlgorithm hash;				///< Hash algorithm
		SigAlgorithm sig_algo;			///< Signature algorithm
		std::vector<char> signature;	///< Signature
	};

	std::vector<SCT> sct_list;

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Authority information access (private extension). https://tools.ietf.org/html/rfc5280#section-4.2.2.1 Partially implemented. */
class ExtAuthorityInformationAccess : public ExtBase
{
public:
	std::string name() const { return "ExtAuthorityInformationAccess"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Certificate policies.

	https://tools.ietf.org/html/rfc5280#section-4.2.1.4.

	certificatePolicies ::= SEQUENCE SIZE (1..MAX) OF PolicyInformation

	PolicyInformation ::= SEQUENCE {
		policyIdentifier   CertPolicyId,
		policyQualifiers   SEQUENCE SIZE (1..MAX) OF
								PolicyQualifierInfo OPTIONAL }

	CertPolicyId ::= OBJECT IDENTIFIER

	PolicyQualifierInfo ::= SEQUENCE {
		policyQualifierId  PolicyQualifierId,
		qualifier          ANY DEFINED BY policyQualifierId }

	Qualifier ::= CHOICE {
		cPSuri           CPSuri,
		userNotice       UserNotice }

	CPSuri ::= IA5String

	UserNotice ::= SEQUENCE {
		noticeRef        NoticeReference OPTIONAL,
		explicitText     DisplayText OPTIONAL }

	NoticeReference ::= SEQUENCE {
		organization     DisplayText,
		noticeNumbers    SEQUENCE OF INTEGER }

*/
class ExtCertificatePolicies : public ExtBase
{
public:
	std::string name() const { return "ExtCertificatePolicies"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	enum class PolicyQualifierId
	{
		cps,			///< Certification practice statement
		unotice,		///< User notice
	};

	struct DisplayText
	{
		enum Type
		{
			ia5_string,
			visible_string,
			bmp_string,
			utf8_string,
		};
		Type type;
		std::string string;

		DisplayText() : type(Type::ia5_string) {}
	};

	struct NoticeReference
	{
		DisplayText organization;
		std::vector<Bignum> notice_numbers;
	};

	struct UserNotice
	{
		NoticeReference notice_ref;
		DisplayText explicit_text;
	};

	struct PolicyQualifierInfo
	{
		PolicyQualifierId id;
		std::string cps_qual;							///< Cps qualifier
		UserNotice user_notice;							///< User notice
	};

	struct PolicyInfo
	{
		oid_value policy_qualifier_id;
		std::vector<PolicyQualifierInfo> policy_qualifiers;
	};

	std::vector<PolicyInfo> certificate_policies;

	//DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Policy mappings. https://tools.ietf.org/html/rfc5280#section-4.2.1.5. Partially implemented, does not interpret values. */
class ExtPolicyMappings : public ExtBase
{
public:
	std::string name() const { return "ExtPolicyMappings"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Subject directory attributes. https://tools.ietf.org/html/rfc5280#section-4.2.1.8 */
class ExtSubjectDirectoryAttributes : public ExtBase
{
public:
	std::string name() const { return "ExtSubjectDirectoryAttributes"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	std::vector<std::pair<AttributeType, DirectoryString>> directory_attributes;

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Name constraints. https://tools.ietf.org/html/rfc5280#section-4.2.1.10 Partially implemented. */
class ExtNameConstraints : public ExtBase
{
public:
	std::string name() const { return "ExtNameConstraints"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Policy constraints. https://tools.ietf.org/html/rfc5280#section-4.2.1.11 Partially implemented. */
class ExtPolicyConstraints : public ExtBase
{
public:
	std::string name() const { return "ExtPolicyConstraints"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** CRL distribution points. https://tools.ietf.org/html/rfc5280#section-4.2.1.13 Partially implemented. */
class ExtCrlDistributionPoints : public ExtBase
{
public:
	std::string name() const { return "ExtCrlDistributionPoints"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Inhibit any policy. https://tools.ietf.org/html/rfc5280#section-4.2.1.14 */
class ExtInhibitAnyPolicy : public ExtBase
{
public:
	std::string name() const { return "ExtInhibitAnyPolicy"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	Bignum val;

	std::string str() const;
	bool parse(std::string&);
	void dump();
};

/** Freshest CRL, aka delta CRL distribution points. https://tools.ietf.org/html/rfc5280#section-4.2.1.15 Partially implemented. */
class ExtFreshestCrl : public ExtBase
{
public:
	std::string name() const { return "ExtFreshestCrl"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};


/** Subject information access (private extension). https://tools.ietf.org/html/rfc5280#section-4.2.2.2 Partially implemented. */
class ExtSubjectInformationAccess : public ExtBase
{
public:
	std::string name() const { return "ExtSubjectInformationAccess"; }
	static const oid_value s_oid;
	oid_value oid() const { return s_oid; }

	DerSequence val;			///< Does not interpret contents.

	std::string str() const;
	bool parse(std::string&);
	void dump();
};


#endif

/** Signature algorithms for X.509 certificates.

	A list of signature algorithms which can be used to sign certificates.

	Algorithms which are unsupported are listed as unknown.

	See:
	- https://tools.ietf.org/html/rfc3279
	- https://tools.ietf.org/html/rfc4055
	- https://tools.ietf.org/html/rfc5758
*/
enum class X509SignatureAlgo : int
{
	unknown 		= 0x0,
	rsa_md5 		= 0x01,		///< {1, 2, 840, 113549, 1, 1, 4}
	rsa_sha1		= 0x02,		///< {1, 2, 840, 113549, 1, 1, 5}
	rsa_sha224		= 0x03,		///< {1, 2, 840, 113549, 1, 1, 14}
	rsa_sha256		= 0x04,		///< {1, 2, 840, 113549, 1, 1, 11}
	rsa_sha384		= 0x05,		///< {1, 2, 840, 113549, 1, 1, 12}
	rsa_sha512		= 0x06,		///< {1, 2, 840, 113549, 1, 1, 13}
	ecdsa_sha1		= 0x10,		///< {1, 2, 840, 10045, 4, 1}
	ecdsa_sha224	= 0x20,		///< {1, 2, 840, 10045, 4, 3, 1}
	ecdsa_sha256	= 0x30,		///< {1, 2, 840, 10045, 4, 3, 2}
	ecdsa_sha384	= 0x40,		///< {1, 2, 840, 10045, 4, 3, 3}
	ecdsa_sha512	= 0x50,		///< {1, 2, 840, 10045, 4, 3, 4}
};

/** X.509 certificate.

	Certificates for use in TLS. Supports only version 3 certificates.

	See:
	- https://tools.ietf.org/html/rfc5280
	Updates:
	- https://tools.ietf.org/html/rfc6818
	- https://tools.ietf.org/html/rfc8398
	- https://tools.ietf.org/html/rfc8399

	Certificate  ::=  SEQUENCE  {
		tbsCertificate       TBSCertificate,
		signatureAlgorithm   AlgorithmIdentifier,
		signatureValue       BIT STRING  }

	TBSCertificate  ::=  SEQUENCE  {
		version         [0]  EXPLICIT Version DEFAULT v1,
		serialNumber         CertificateSerialNumber,
		signature            AlgorithmIdentifier,
		issuer               Name,
		validity             Validity,
		subject              Name,
		subjectPublicKeyInfo SubjectPublicKeyInfo,
		issuerUniqueID  [1]  IMPLICIT UniqueIdentifier OPTIONAL,
								-- If present, version MUST be v2 or v3
		subjectUniqueID [2]  IMPLICIT UniqueIdentifier OPTIONAL,
								-- If present, version MUST be v2 or v3
		extensions      [3]  EXPLICIT Extensions OPTIONAL
								-- If present, version MUST be v3 }
*/
struct X509Cert
{
	X509Cert() {}
	virtual ~X509Cert() {}

	Bignum serial_number;								///< The certificate serial number.
	std::vector<RelativeDistinguishedName> issuer;		///< Issuer name.
	std::chrono::system_clock::time_point valid_start;	///< Time before which this certificate is invalid.
	std::chrono::system_clock::time_point valid_end;	///< Time after which this certificate is invalid.
	std::vector<RelativeDistinguishedName> subject;		///< Subject name.
	std::vector<char> issuer_unique_id;					///< Issuer unique id. Optional: size 0 means not present.
	std::vector<char> subject_unique_id;				///< Subject unique id. Optional: size 0 means not present.
	std::vector<ExtBasePtr> extensions;					///< Extensions.

	ExtBasePtr find_ext(ExtType t) const
	{
		auto it = std::find_if(extensions.begin(), extensions.end(), [&t](auto& i)
		{
			return i->oid == ExtBase::find_oid(ExtType::subject_alternative_name);
		});
		ExtBasePtr ret;
		if (it != extensions.end())	ret = *it;
		return ret;
	}

	PublicKeyCert public_key;							///< Certificate owner's public key.

	std::vector<char> cert_bin;							///< Binary form of certificate from latest dump() or parse(), signed by the issuer.
	oid_value sig_algo_oid;								///< Algorithm used to sign this certificate.
	oid_value sig_algo_params;							///< Signature algorithm parameters.
	std::vector<uint8_t> signature;						///< Digital signature of this certificate signed using the issuer's private key.

	bool bin_compare(const X509Cert& other) const
	{
		return cert_bin == other.cert_bin;
	}

	X509SignatureAlgo sig_algo() const;

	/** Parse from a document.
	
		Resets certificate binary.

		Throws an exception if this is not a version 3 X.509 certificate.
	*/
	void parse(const DerDocument&);

	/** Descriptive string.

		Debug dumps the asn.1 for all extensions.
	*/
	std::string str(bool = false) const;

	/** Validate this certificate against another.
	
		Returns true if the private key associated with the issuer certificate public key was used to sign this certificate.
	*/
	bool validate(const X509Cert&) const;

	/** Validate this certificate against it's own public key.

		Returns true if this is a self-signed certificate. Root certificates ("trust anchors"), are also self-signed.
	*/
	bool validate() const
	{
		return validate(*this);
	}

	/**	Validate signature against an RSA public key.
		\param key RSA public key.

		Returns false if signature algorithm is not RSA, is unknown, or validate fails.
	*/
	bool validate(const RsaPublicKey&) const;
	
	/**	Validate signature against an ECDSA algorithm and public key.
		\param curve EC curve type.
		\param key EC public key.

		Returns false if signature algorithm is not ECDSA, is unknown, or validate fails.

		If the point is invalid, throws an exception.
	*/
	bool validate(const EccGfpPoint&) const;

	/** Dump the certificate to a sequence. */
	BasePtr dump_cert() const;

	/** Sign the certificate and dump to a document using the RSA signature algorithm.

		\param doc Document to dump output.
		\param key RSA private key.
		\param algo Signature algorithm. Throws exception if this is not an rsa_ signature algorithm.

		This will set the certificate algorithm, certificate binary, and signature, then dump the document.
	*/
	void sign_and_dump(DerDocument&, const RsaPrivateKey&, const X509SignatureAlgo&);

	/** Sign the certificate and dump to a document using the ECDSA signature algorithm.

		\param doc Document to dump output.
		\param key_type The curve type. Must be an ecdsa_ key type.
		\param reg_key Regular private key. Signature will be verified using the public key associated with this key.
		\param tmp_key Temporary private key. 
		\param sig_algo Signature algorithm. Throws exception if this is not an ecdsa_ signature algorithm.

		This will set the certificate algorithm, certificate binary, and signature.

		Throws exception if inconsistent keys are input.
	*/
	void sign_and_dump(DerDocument&, const KeyAlgoType&, const Bignum&, Bignum&, const X509SignatureAlgo&);

};

/** Certificate bundle. This is a bundle of X.509 certificates.

	Can be used to build up a list of trusted certificates, or "trust anchors".

	Input files must be PEM-formatted, with certificates concatenated as follows:
	-----BEGIN CERTIFICATE-----
	<cert 1>
	-----END CERTIFICATE-----
	-----BEGIN CERTIFICATE-----
	<cert 2>
	-----END CERTIFICATE-----
	etc.
*/
struct CertBundle : std::vector<std::shared_ptr<X509Cert>>
{
	CertBundle() {}
	CertBundle(std::istream& s)
	{
		parse(s);
	}
	virtual ~CertBundle() {}

	/** Parse bundle from stream. Reads certificates until eof and adds them to the bundle.
	
		Throws exception on any parse error (other than stream eof).
	*/
	void parse(std::istream& s);
};

/** @} */
/** @} */

}	// namespace

std::ostream& operator<<(std::ostream&, const scc::crypto::X509Cert&);
std::ostream& operator<<(std::ostream&, const scc::crypto::KeyAlgoType&);
std::ostream& operator<<(std::ostream&, const scc::crypto::X509SignatureAlgo&);

#endif
