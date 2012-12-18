#!/usr/bin/env ruby
#
# Copyright (c) 2011 Mark Heily <mark@heily.com>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

$LOAD_PATH << 'makeconf'

require 'makeconf'

cc = CCompiler.new(
  :search => %w{clang}
)

project = Project.new(
  :id => 'libBlocksRuntime',
  :version => '0.3',
  :cc => cc
)

project.add(
  Library.new(
      :id => 'libBlocksRuntime', 
      :cflags => '-DBlocksRuntime_EXPORTS -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_INT -DHAVE_SYNC_BOOL_COMPARE_AND_SWAP_LONG -std=c99 -Wall -Wextra -W -pedantic -Wno-unused-parameter',
      :sources => %w{runtime.c data.c}
  ),
  Header.new( :id => 'headers', :sources => %w{ Block.h Block_private.h } ),
  Test.new(
    :id => 'blktest', 
    :cflags => '-I. -fblocks',
    :sources => %w{test/main.c},
    :ldadd => 'libBlocksRuntime.a'
  )
)

# Require the use of an unofficial Clang toolchain for Android.
if SystemType.host =~ /-androideabi$/
  project.ndk_toolchain_version = 'clang3.1'
end

mc = Makeconf.new
mc.configure(project)


#package:
#    summary: "Blocks Runtime library"
#    description: "Blocks Runtime library"
#    license: LLVM
#    author: "LLVM Development Team"
