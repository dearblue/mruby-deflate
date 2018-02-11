 #!ruby

module Deflate
  FORMAT_DEFLATE = :deflate
  FORMAT_ZLIB = :zlib
  FORMAT_GZIP = :gzip

  #
  # call-seq:
  #   encode(src, maxdest = nil, dest = nil, level: DEFAULT_COMPRESSION, format: :gzip)
  #
  def Deflate.encode(src, *args)
    Encoder.encode(src, *args)
  end

  #
  # call-seq:
  #   decode(src, maxdest, dest = nil, format: :gzip)
  #
  def Deflate.decode(src, *args)
    Decoder.decode(src, *args)
  end

  Compressor = Encoder
  Decompressor = Decoder

  class << self
    alias deflate encode
    alias compress encode
    alias inflate decode
    alias decompress decode
    alias uncompress decode
  end

  class << Encoder
    alias compress encode
    alias deflate encode
  end

  class << Decoder
    alias inflate decode
    alias decompress decode
    alias uncompress decode
  end
end
