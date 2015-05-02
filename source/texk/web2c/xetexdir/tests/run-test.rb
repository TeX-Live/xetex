#!/usr/bin/env ruby

srcdir = File.expand_path('../../../../kpathsea', __FILE__)
ENV['TEXMFCNF'] = srcdir
ENV['TEXINPUTS'] = File.expand_path('../../../tests', __FILE__) + ':'
xetexbin = File.expand_path('../../../../../../build/texk/web2c/xetex', __FILE__)
system("#{xetexbin} -etex -ini -no-pdf #{ARGV.first}")
