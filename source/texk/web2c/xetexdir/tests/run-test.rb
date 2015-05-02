#!/usr/bin/env ruby

srcdir = File.expand_path('../../../../kpathsea', __FILE__)
ENV['TEXMFCNF'] = srcdir
ENV['TEXINPUTS'] = ['../../../tests', '../../../xetexdir/tests'].map do |relpath|
  File.expand_path(relpath, __FILE__)
end.join(':') + ':'
xetexbin = File.expand_path('../../../../../../build/texk/web2c/xetex', __FILE__)
system("#{xetexbin} -etex -ini -no-pdf #{ARGV.first}")
