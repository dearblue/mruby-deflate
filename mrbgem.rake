#!ruby

MRuby::Gem::Specification.new("mruby-deflate") do |s|
  s.summary = "libdeflate bindings for mruby"
  s.version = "0.1"
  s.license = "BSD-2-Clause"
  s.author  = "dearblue"
  s.homepage = "https://github.com/dearblue/mruby-deflate"

  add_dependency "mruby-error",        core: "mruby-error"
  add_dependency "mruby-string-ext",   core: "mruby-string-ext"
  add_dependency "mruby-aux",          github: "dearblue/mruby-aux"

  cc.defines << "SUPPORT_NEAR_OPTIMAL_PARSING=1"

  dirp = dir.gsub(/[\[\]\{\}\,]/) { |m| "\\#{m}" }
  files = "contrib/libdeflate/lib/**/*.c"
  objs.concat(Dir.glob(File.join(dirp, files)).map { |f|
    next nil unless File.file? f
    objfile f.relative_path_from(dir).pathmap("#{build_dir}/%X")
  }.compact)

  inc = [
    "contrib/libdeflate",
    "contrib/libdeflate/common", # for contrib/libdeflate/lib
  ]

  cc.include_paths.concat inc
end
