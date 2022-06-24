project := libhttp
summary := A cURL-based HTTP client library for C++.

STD := c++20

curl = curl-config
curl.libs = $(subst -l,,$(shell $(curl) --libs))

library = $(project)
$(library).type = shared
$(library).libs = $(curl.libs) ext++ fmt timber

test.deps = http
test.libs = fmt gtest gtest_main http

install := $(library)
targets := $(install)

install.directories = $(include)/http

include mkbuild/base.mk
