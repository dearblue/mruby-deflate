#!ruby

# NOTE: FrozenError is defined by mruby-1.4 or later.
FrozenError ||= RuntimeError

# "".deflate
empty_deflate = "\x03\x00"

# "a".deflate
a_deflate = "\x4B\x04\x00"

# "abcdefghijklmnopqrstuvwxyz".deflate
atoz_deflate =
  "\x4B\x4C\x4A\x4E\x49\x4D\x4B\xCF\xC8\xCC\xCA\xCE\xC9\xCD\xCB\x2F" <<
  "\x28\x2C\x2A\x2E\x29\x2D\x2B\xAF\xA8\xAC\x02\x00"

# ("a" * 33706430) を deflate (level=6) で二重圧縮した結果
# 33706430 byte => 32766 byte => 179 byte になった
a33706430_deflate_deflate =
  "\xED\xDD\xAD\x0E\x81\x01\x14\x06\x60\x46\x12\xFC\x04\x8A\xA2\xB8" <<
  "\x07\xC5\x46\x91\x05\x9A\xCD\x5C\x80\xB9\x04\xCC\x5C\x8A\xE0\x02" <<
  "\x74\x9A\x2B\xF0\xD3\x04\x51\x12\x24\xE6\xE7\x1B\x97\xF0\xF9\x36" <<
  "\x9E\x27\x9D\xF8\xE6\xB3\xB3\xF7\x9C\x56\xE3\xD8\xD3\xA8\xB4\xB9" <<
  "\x95\x0B\xF3\x54\x0C\x00\x00\x00\xF8\x71\xA3\xFD\x24\x11\x7F\x0D" <<
  "\xA5\xFB\xB6\xD1\x6C\x03\x00\x00\x00\xBF\xAE\x97\x49\x27\x83\xA5" <<
  "\xC0\xB5\x5B\xAC\x47\x1D\x06\x00\x00\x00\x08\xDD\xB4\x5A\xC9\x06" <<
  "\x17\x02\xE7\x61\x3F\x19\x75\x18\x00\x00\x00\x20\x74\xF9\xCB\xA1" <<
  "\x16\xEC\x02\xD6\xF7\x45\xAE\x15\x75\x1A\x00\x00\x00\x20\x74\x3B" <<
  "\x85\x01\x00\x00\x00\xF0\x57\x14\x06\x00\x00\x00\xC0\x7F\x99\xBD" <<
  "\x0B\x03\xB2\xC7\x41\x75\x99\xCA\x7F\xFD\x93\x01\x00\x00\x00\xF0" <<
  "\xD1\x79\x00"

atoz_zlib =
  "\x78\x9C\x4B\x4C\x4A\x4E\x49\x4D\x4B\xCF\xC8\xCC\xCA\xCE\xC9\xCD" <<
  "\xCB\x2F\x28\x2C\x2A\x2E\x29\x2D\x2B\xAF\xA8\xAC\x02\x00\x90\x86" <<
  "\x0B\x20"

atoz_gzip =
  "\x1F\x8B\x08\x00\x00\x00\x00\x00\x00\x03\x4B\x4C\x4A\x4E\x49\x4D" <<
  "\x4B\xCF\xC8\xCC\xCA\xCE\xC9\xCD\xCB\x2F\x28\x2C\x2A\x2E\x29\x2D" <<
  "\x2B\xAF\xA8\xAC\x02\x00\xBD\x50\x27\x4C\x1A\x00\x00\x00"

atoz = "abcdefghijklmnopqrstuvwxyz"
atoz1000 = atoz * 1000

dest = ""

assert("one-shot Deflate.decode (inflate)") do
  assert_raise(ArgumentError) { Deflate.inflate }
  assert_raise(ArgumentError) { Deflate.inflate("") }

  assert_equal "", Deflate.inflate(empty_deflate, 50)
  assert_equal "a", Deflate.inflate(a_deflate, 50)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50)
  assert_raise(RuntimeError) { Deflate.inflate(atoz_deflate, 10) }
  assert_raise(RuntimeError) { Deflate.inflate("", 10) }
  assert_raise(TypeError) { Deflate.inflate(10, 10) }
  assert_raise(TypeError) { Deflate.inflate("", "a") }

  assert_equal "", Deflate.inflate(empty_deflate, 50, dest)
  assert_equal "a", Deflate.inflate(a_deflate, 50, dest)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50, dest)
  assert_equal dest.object_id, Deflate.inflate(empty_deflate, 50, dest).object_id
  assert_equal dest.object_id, Deflate.inflate(a_deflate, 50, dest).object_id
  assert_equal dest.object_id, Deflate.inflate(atoz_deflate, 50, dest).object_id
  assert_raise(FrozenError) { Deflate.inflate(empty_deflate, 50, "".freeze) }
  assert_raise(TypeError) { Deflate.inflate(empty_deflate, 50, Object.new) }

  assert_raise(ArgumentError) { Deflate.inflate(empty_deflate, 50, dest, Object.new) }

  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50, format: nil)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50, format: :deflate)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_zlib, 50, format: :zlib)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_gzip, 50, format: :gzip)
  assert_raise(ArgumentError) { Deflate.inflate(empty_deflate, 50, format: :wrong_format) }

  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50, dest, format: nil)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_deflate, 50, dest, format: :deflate)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_zlib, 50, dest, format: :zlib)
  assert_equal "abcdefghijklmnopqrstuvwxyz", Deflate.inflate(atoz_gzip, 50, dest, format: :gzip)
  assert_raise(ArgumentError) { Deflate.inflate(empty_deflate, 50, dest, format: :wrong_format) }
end

assert("one-shot Deflate.encode (deflate)") do
  assert_raise(ArgumentError) { Deflate.deflate }

  assert_equal "", Deflate.inflate(Deflate.deflate(""), 50)
  assert_equal "a", Deflate.inflate(Deflate.deflate("a"), 50)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz), 50)
  assert_equal atoz1000.hash, Deflate.inflate(Deflate.deflate(atoz1000), atoz1000.size).hash

  assert_equal "", Deflate.inflate(Deflate.deflate("", nil), 50)
  assert_equal "a", Deflate.inflate(Deflate.deflate("a", nil), 50)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, nil), 50)
  assert_equal atoz1000.hash, Deflate.inflate(Deflate.deflate(atoz1000, nil), atoz1000.size).hash
  assert_equal "", Deflate.inflate(Deflate.deflate("", 50), 50)
  assert_equal "a", Deflate.inflate(Deflate.deflate("a", 50), 50)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, 50), 50)
  assert_equal atoz1000.hash, Deflate.inflate(Deflate.deflate(atoz1000, 1000), atoz1000.size).hash
  assert_raise(RuntimeError) { Deflate.deflate(atoz1000, -10) }
  assert_raise(RuntimeError) { Deflate.deflate(atoz1000, 10) }
  assert_equal dest.object_id, Deflate.deflate(atoz1000, dest).object_id
  assert_raise(TypeError) { Deflate.deflate(atoz1000, Object.new) }
  assert_raise(FrozenError) { Deflate.deflate(atoz, "".freeze) }

  assert_equal "", Deflate.inflate(Deflate.deflate("", 50, nil), 50)
  assert_equal "a", Deflate.inflate(Deflate.deflate("a", 50, nil), 50)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, 50, nil), 50)
  assert_equal atoz1000.hash, Deflate.inflate(Deflate.deflate(atoz1000, 1000, nil), atoz1000.size).hash
  assert_equal "", Deflate.inflate(Deflate.deflate("", 50, dest), 50)
  assert_equal "a", Deflate.inflate(Deflate.deflate("a", 50, dest), 50)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, 50, dest), 50)
  assert_equal dest.object_id, Deflate.deflate(atoz, 50, dest).object_id
  assert_raise(TypeError) { Deflate.deflate(atoz1000, 50, Object.new) }
  assert_raise(FrozenError) { Deflate.deflate(atoz, 50, "".freeze) }

  assert_raise(ArgumentError) { Deflate.deflate(atoz, nil, nil, nil) }

  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, format: nil), 50, format: nil)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, format: nil), 50, format: :deflate)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, format: :deflate), 50, format: :deflate)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, format: :zlib), 50, format: :zlib)
  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, format: :gzip), 50, format: :gzip)
  assert_raise(ArgumentError) { Deflate.deflate(atoz, format: :wrong_format) }

  assert_equal atoz, Deflate.inflate(Deflate.deflate(atoz, nil, nil, format: nil), 50, format: nil)
end

__END__

ruby25 -r zlib -e 'class String; def deflate(level = Zlib::DEFAULT_COMPRESSION); Zlib::Deflate.new(level, -15).deflate(self, Zlib::FINISH); end; def to_hex; unpack("H*")[0].upcase.gsub(/(?=(?:[0-9a-f]{2})+\z)/i, "\\\\x"); end; def zlib(level = Zlib::DEFAULT_COMPRESSION); Zlib::Deflate.new(level).deflate(self, Zlib::FINISH); end; def gzip; Zlib::GzipWriter.wrap(StringIO.new(d = "")) { |gz| gz << self; }; d; end; end; puts "abcdefghijklmnopqrstuvwxyz".deflate.to_hex, "abcdefghijklmnopqrstuvwxyz".zlib.to_hex, "abcdefghijklmnopqrstuvwxyz".gzip.to_hex'
