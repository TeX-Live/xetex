#!/usr/bin/env ruby

la = if ARGV.first == '-l' then 'la' else '' end
if ARGV.first == '-l' then filename = ARGV[1] else filename = ARGV.first end

puts "Filename is #{filename}"

srcdir = File.expand_path('../../../../kpathsea', __FILE__)
ENV['TEXMFCNF'] = srcdir
ENV['TEXINPUTS'] = ['../../../tests', '../../../xetexdir/tests'].map do |relpath|
  File.expand_path(relpath, __FILE__)
end.join(':') + ':'
ENV['OSFONTDIR'] = File.expand_path('../fonts', __FILE__)
ENV['TFMFONTS'] = '/usr/local/texlive/2015/texmf-dist/fonts/tfm//:'
puts "export TEXMFCNF=#{ENV['TEXMFCNF']}"
puts "export TEXINPUTS=#{ENV['TEXINPUTS']}"
puts "export OSFONTDIR=#{ENV['OSFONTDIR']}"
puts "export TFMFONTS=#{ENV['TFMFONTS']}"
xetexbin = File.expand_path("../../../../../../build/texk/web2c/xe#{la}tex", __FILE__)
puts "Running: #{xetexbin} -etex -ini -no-pdf #{filename}"
system("#{xetexbin} -etex -ini -no-pdf #{filename}")
