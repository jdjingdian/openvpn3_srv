//    OpenVPN -- An application to securely tunnel IP networks
//               over a single port, with support for SSL/TLS-based
//               session authentication and key exchange,
//               packet encryption, packet authentication, and
//               packet compression.
//
//    Copyright (C) 2012-2017 OpenVPN Inc.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License Version 3
//    as published by the Free Software Foundation.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program in the COPYING file.
//    If not, see <http://www.gnu.org/licenses/>.

// API for SSL implementations

#ifndef OPENVPN_SSL_SSLAPI_H
#define OPENVPN_SSL_SSLAPI_H

#include <string>
#include <cstdint>

#include <openvpn/common/size.hpp>
#include <openvpn/common/exception.hpp>
#include <openvpn/common/rc.hpp>
#include <openvpn/common/options.hpp>
#include <openvpn/common/mode.hpp>
#include <openvpn/buffer/buffer.hpp>
#include <openvpn/frame/frame.hpp>
#include <openvpn/auth/authcert.hpp>
#include <openvpn/pki/epkibase.hpp>
#include <openvpn/ssl/kuparse.hpp>
#include <openvpn/ssl/nscert.hpp>
#include <openvpn/ssl/tlsver.hpp>
#include <openvpn/ssl/tls_remote.hpp>
#include <openvpn/ssl/tls_cert_profile.hpp>
#include <openvpn/random/randapi.hpp>

namespace openvpn {

  class SSLAPI : public RC<thread_unsafe_refcount>
  {
  public:

    enum TLSWarnings {
      TLS_WARN_SIG_MD5 = (1 << 0),
      TLS_WARN_NAME_CONSTRAINTS = (1 << 1)
    };

    typedef RCPtr<SSLAPI> Ptr;

    virtual void start_handshake() = 0;
    virtual ssize_t write_cleartext_unbuffered(const void *data, const size_t size) = 0;
    virtual ssize_t read_cleartext(void *data, const size_t capacity) = 0;
    virtual bool read_cleartext_ready() const = 0;
    virtual void write_ciphertext(const BufferPtr& buf) = 0;
    virtual bool read_ciphertext_ready() const = 0;
    virtual BufferPtr read_ciphertext() = 0;
    virtual std::string ssl_handshake_details() const = 0;
    virtual const AuthCert::Ptr& auth_cert() const = 0;
    uint32_t get_tls_warnings() const
    {
      return tls_warnings;
    }
  protected:
    uint32_t tls_warnings = 0; // bitfield of SSLAPI::TLSWarnings
  };

  class SSLFactoryAPI : public RC<thread_unsafe_refcount>
  {
  public:
    OPENVPN_EXCEPTION(ssl_options_error);
    OPENVPN_EXCEPTION(ssl_context_error);
    OPENVPN_EXCEPTION(ssl_external_pki);
    OPENVPN_SIMPLE_EXCEPTION(ssl_ciphertext_in_overflow);

    typedef RCPtr<SSLFactoryAPI> Ptr;

    // create a new SSLAPI instance
    virtual SSLAPI::Ptr ssl() = 0;

    // like ssl() above but verify hostname against cert CommonName and/or SubjectAltName
    virtual SSLAPI::Ptr ssl(const std::string& hostname) = 0;

    // client or server?
    virtual const Mode& mode() const = 0;
  };

  class SSLConfigAPI : public RC<thread_unsafe_refcount>
  {
  public:
    typedef RCPtr<SSLConfigAPI> Ptr;

    enum PKType {
      PK_UNKNOWN = 0,
      PK_NONE,
      PK_RSA,
      PK_ECKEY,
      PK_ECKEY_DH,
      PK_ECDSA,
      PK_RSA_ALT,
      PK_RSASSA_PSS,
    };

    enum LoadFlags {
      LF_PARSE_MODE                     = (1<<0),
      LF_ALLOW_CLIENT_CERT_NOT_REQUIRED = (1<<1),
      LF_RELAY_MODE                     = (1<<2), // look for "relay-ca" instead of "ca" directive
      LF_ALLOW_NAME_CONSTRAINTS         = (1<<3)  // do not fail on Name Constraints ext and drop a warning to UI
    };

    std::string private_key_type_string() const
    {
      PKType type = private_key_type();

      switch (type)
      {
      case PK_NONE:
	return "None";
      case PK_RSA:
	return "RSA";
      case PK_ECKEY:
	return "EC";
      case PK_ECKEY_DH:
	return "EC_DH";
      case PK_ECDSA:
	return "ECDSA";
      case PK_RSA_ALT:
	return "RSA_ALT";
      case PK_RSASSA_PSS:
	return "RSASSA_PSS";
      case PK_UNKNOWN:
      default:
	return "Unknown";
      }
    }

    virtual void set_mode(const Mode& mode_arg) = 0;
    virtual const Mode& get_mode() const = 0;
    virtual void set_external_pki_callback(ExternalPKIBase* external_pki_arg) = 0; // private key alternative
    virtual void set_private_key_password(const std::string& pwd) = 0;
    virtual void load_ca(const std::string& ca_txt, bool strict) = 0;
    virtual void load_crl(const std::string& crl_txt) = 0;
    virtual void load_cert(const std::string& cert_txt) = 0;
    virtual void load_cert(const std::string& cert_txt, const std::string& extra_certs_txt) = 0;
    virtual void load_private_key(const std::string& key_txt) = 0;
    virtual void load_dh(const std::string& dh_txt) = 0;
    virtual std::string extract_ca() const = 0;
    virtual std::string extract_crl() const = 0;
    virtual std::string extract_cert() const = 0;
    virtual std::vector<std::string> extract_extra_certs() const = 0;
    virtual std::string extract_private_key() const = 0;
    virtual std::string extract_dh() const = 0;
    virtual PKType private_key_type() const = 0;
    virtual size_t private_key_length() const = 0;
    virtual void set_frame(const Frame::Ptr& frame_arg) = 0;
    virtual void set_debug_level(const int debug_level) = 0;
    virtual void set_flags(const unsigned int flags_arg) = 0;
    virtual void set_ns_cert_type(const NSCert::Type ns_cert_type_arg) = 0;
    virtual void set_remote_cert_tls(const KUParse::TLSWebType wt) = 0;
    virtual void set_tls_remote(const std::string& tls_remote_arg) = 0;
    virtual void set_tls_version_min(const TLSVersion::Type tvm) = 0;
    virtual void set_tls_version_min_override(const std::string& override) = 0;
    virtual void set_tls_cert_profile(const TLSCertProfile::Type type) = 0;
    virtual void set_tls_cert_profile_override(const std::string& override) = 0;
    virtual void set_local_cert_enabled(const bool v) = 0;
    virtual void set_enable_renegotiation(const bool v) = 0;
    virtual void set_force_aes_cbc_ciphersuites(const bool v) = 0;
    virtual void set_x509_track(X509Track::ConfigSet x509_track_config_arg) = 0;
    virtual void set_rng(const RandomAPI::Ptr& rng_arg) = 0;
    virtual void load(const OptionList& opt, const unsigned int lflags) = 0;

    virtual std::string validate_cert(const std::string& cert_txt) const = 0;
    virtual std::string validate_cert_list(const std::string& certs_txt) const = 0;
    virtual std::string validate_crl(const std::string& crl_txt) const = 0;
    virtual std::string validate_private_key(const std::string& key_txt) const = 0;
    virtual std::string validate_dh(const std::string& dh_txt) const = 0;

    virtual SSLFactoryAPI::Ptr new_factory() = 0;
  };
}

#endif
