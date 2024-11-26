Name:           hashrat
Version:        1.19
Release:        1%{?dist}
Summary:        A hash-generation utility

License:        GPLv3
URL:            https://github.com/ColumPaget/Hashrat/
Source0:        https://github.com/ColumPaget/Hashrat/archive/v%{version}.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires:  gcc

%description
Hashrat is a hash-generation utility that supports the md5, sha1,
sha256, sha512, whirlpool, jh-244, jh256, jh-384 and jh-512 hash
functions, and also the HMAC versions of those functions. 
It can output in 'traditional' format (same as md5sum and shasum and the
like), or its own format. Hashes can be output in octal, decimal,
hexadecimal, uppercase hexadecimal and varieties of base32 and base64.
It can be used as a google-authenticator compabile TOTP client.
It supports directory recursion, hashing entire devices, and generating 
a hash for an entire directory. 
It has a 'CGI' mode that can be used as a web-page to lookup hashes.

%prep
%autosetup -p1 -n Hashrat-%{version}

%build
%configure
%make_build

%install
%make_install

%check
make check

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.1*

%changelog
* Thu Nov 19 2020 Joel Barrios <http://www.alcancelibre.org/> - 3.1-1
- Initial spec file.
