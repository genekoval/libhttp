project := libhttp
summary := A cURL-based HTTP client library for C++.

STD := c++20

curl = curl-config
curl.libs = $(subst -l,,$(shell $(curl) --libs))

common.libs = $(curl.libs) ext++ fmt netcore timber

library = $(project)
$(library).type = shared
$(library).libs = $(common.libs)

test.deps = $(project)
test.libs = $(common.libs) gtest http

install := $(library)
targets := $(install)

install.directories = $(include)/http

files = $(include) $(src) test Makefile VERSION

include mkbuild/base.mk

defines.develop = TIMBER_TIMER_ACTIVE
