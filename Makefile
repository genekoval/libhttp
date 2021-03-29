project := libhttp
summary := A cURL-based HTTP client library for C++.

STD := c++20

curl = curl-config
curl.libs = $(subst -l,,$(shell $(curl) --libs))

library = $(project)
$(library).type = shared
define $(library).libs
 $(curl.libs)
 ext++
 fmt
endef

define test.libs
 gtest
 gtest_main
 http
endef

install := $(library)
targets := $(install)

include mkbuild/base.mk
