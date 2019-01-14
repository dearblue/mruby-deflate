# mruby-deflate - libdeflate bindings for mruby

mruby へ deflate 圧縮機能を提供します。

deflate/zlib/gzip データのメモリ間圧縮・伸長が行なえます。

libdeflate ライブラリを利用しています。


## LIMITATION

  * mruby のメモリアロケータを無視し、標準 C で定義される malloc/free が使われます。
  * ストリーミング処理は出来ません。


## HOW TO USAGE

``build_config.rb`` に gem として追加して、mruby をビルドして下さい。

```ruby
MRuby::Build.new do |conf|
  conf.gem "mruby-deflate", github: "dearblue/mruby-deflate"
end
```

### 圧縮 (one-shot compression)

```ruby
input = "abcdefg"
output = Deflate.encode(input)
```

圧縮レベルや出力形式は、level および format キーワード引数で指定します。
また、任意で最大出力長と、出力先として使いまわしの出来る文字列オブジェクトの指定が可能です。

```ruby
input = "abcdefg"
maxoutsize = 50
output = ""
level = :max
format = :gzip
output = Deflate.encode(input, maxoutsize, output, level: level, format: format)
```

  * ``Deflate.encode(input, maxout = nil, output = nil, **opts) -> output``<br>
    ``Deflate.encode(input, output, **opts) -> output``
      * 戻り値:: 引数で指定した output を返す。output を省略か nil を与えた場合は文字列オブジェクトを返す。
      * 引数 ``input``:: 圧縮したい、バイナリデータとみなされる文字列オブジェクトを指定する。
      * 引数 ``maxout``:: 圧縮後のする最大バイト長を指定する。nil を与えた場合は input の長さから自動で決定される。
      * 引数 ``output``:: 圧縮されたバイナリデータを格納する文字列オブジェクト。nil を与えた場合は、内部でからの文字列オブジェクトが用意される。
      * 引数 ``opts``:: キーワード引数
          * ``level: 6``:: 1..12, ``:min``, ``:max``, ``:fast``, ``:best`` or ``nil``
          * ``format: :deflate``:: ``:deflate``, ``:zlib``, ``:gzip``

### 伸長 (one-shot decompression)

```ruby
input_data = ...
max_output = 1000
output = "" # 使い回しのためのオブジェクト (任意)
output = Deflate.decode(input_data, max_output, output)
```

伸長後のバイト長が ``max_output`` を超える場合、例外が発生します。途中までの伸長処理は行なえません。

  * ``Deflate.decode(input, maxout = nil, output = nil, **opts) -> output``
      * 戻り値:: 伸長した結果を格納した、引数で指定した output を返す。
      * 引数 input:: brotli で圧縮したバイナリデータとしての文字列オブジェクト。
      * 引数 maxout:: 伸長する最大バイト長。
      * 引数 output:: 伸長後のデータを格納する文字列オブジェクト。省略、あるいは nil を与えた場合は内部で生成される。
      * 引数 opts:: キーワード引数
          * ``format: :deflate``:: ``:deflate``, ``:zlib``, ``:gzip``


## Specification

  * Product name: [mruby-deflate](https://github.com/dearblue/mruby-deflate)
  * Version: 0.1
  * Product quality: PROTOTYPE, UNSTABLE, EXPERIMENTAL
  * Author: [dearblue](https://github.com/dearblue)
  * Project page: <https://github.com/dearblue/mruby-deflate>
  * Licensing: [2 clause BSD License](LICENSE)
  * Dependency external mrbgems: (NONE)
  * Language feature requirements:
      * generic selection (C11)
      * compound literals (C99)
      * flexible array (C99)
      * variable length arrays (C99)
      * designated initializers (C99)
  * Dependency external mrbgems:
      * [mruby-aux](https://github.com/dearblue/mruby-aux)
        under [2 clause BSD License](https://github.com/dearblue/mruby-aux/blob/master/LICENSE)
        by [dearblue](https://github.com/dearblue)
  * Bundled C libraries (git-submodules):
      * [libdeflate](https://github.com/ebiggers/libdeflate)
        [version 1.1](https://github.com/ebiggers/libdeflate/tree/v1.1)
        under [MIT License](https://github.com/ebiggers/libdeflate/blob/v1.1/COPYING)
        by [ebiggers](https://github.com/ebiggers)
