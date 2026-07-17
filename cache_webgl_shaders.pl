#!/usr/bin/env perl

use strict;
use warnings;
use FindBin qw($Bin);
use File::Path qw(make_path);
use File::Spec;

my $shader_dir = File::Spec->catdir($Bin, 'data', 'shaders');
my $source_index = File::Spec->catfile($shader_dir, 'index.txt');
my $cache_dir = File::Spec->catdir($shader_dir, 'webgl_cache');
my $cache_index = File::Spec->catfile($cache_dir, 'index.txt');

sub read_file {
    my ($path) = @_;
    open my $file, '<:raw', $path or die "Unable to read $path: $!\n";
    local $/;
    my $contents = <$file>;
    close $file;
    return $contents;
}

sub write_if_changed {
    my ($path, $contents) = @_;
    if (-f $path && read_file($path) eq $contents) {
        return 0;
    }
    open my $file, '>:raw', $path or die "Unable to write $path: $!\n";
    print {$file} $contents;
    close $file;
    return 1;
}

sub uses_unsupported_inputs {
    my ($filename, $source) = @_;
    return 1 if $filename =~ /(?:cache|audio)/;
    return 1 if $source =~ /(?:sampler1D|spectrum|audioData|audio_data|fft|FFT)/;
    return 1 if $source =~ /\buniform\s+float\s+[^;]*\b(?:amp|amp_peak|amp_rms|amp_smooth|amp_low|amp_mid|amp_high|iamp)\b[^;]*;/;
    return 1 if $source =~ /\buniform\s+sampler2D\s+[^;]*(?:\bsamp[1-9][0-9]*\b|\btextures\s*\[)[^;]*;/;
    return 0;
}

sub make_fragment_output_safe {
    my ($source) = @_;
    return $source unless $source =~ /\bout\s+vec4\s+([A-Za-z_][A-Za-z0-9_]*)\s*;/;
    my $output_name = $1;
    return $source unless $source =~ /\b\Q$output_name\E\s*\[\s*[A-Za-z_]/;

    $source =~ s/\bout\s+vec4\s+\Q$output_name\E\s*;/out vec4 mxWebGLFragColor;\nvec4 mxOutputColor;/;
    $source =~ s/\b\Q$output_name\E\b/mxOutputColor/g;
    $source =~ s/\bvoid\s+main\s*\(\s*(?:void)?\s*\)/void mxShaderMain()/;
    $source .= "\nvoid main() {\n    mxShaderMain();\n    mxWebGLFragColor = mxOutputColor;\n}\n";
    return $source;
}

sub normalize_float_literals {
    my ($source) = @_;
    my $operator = qr/(?:\*=|\/=|\+=|-=|[+*\/<>=]=?|==|!=|-)/;
    my $suffix = qr/(?:\s*(?:\.[A-Za-z]+|\[[^\]\r\n]+\]))?/;
    my $needs_conversion = $source =~ /\b(?:mod|pingPong)\s*\([^,\r\n]+,\s*-?[0-9]+\s*\)/;
    for my $name (qw(tc time_f iTime iResolution iMouse color)) {
        $needs_conversion ||= $source =~ /\b\Q$name\E\b$suffix\s*$operator\s*-?[0-9]+(?![.A-Za-z0-9_])/;
        $needs_conversion ||= $source =~ /(^|[^A-Za-z0-9_.])-?[0-9]+\s*$operator\s*\Q$name\E\b/m;
    }
    return $source unless $needs_conversion;

    my %float_name = map { $_ => 1 }
        qw(tc TexCoord time_f iTime iResolution iMouse mxOutputColor);
    while ($source =~ /\b(?:float|vec[234])\s+([A-Za-z_][A-Za-z0-9_]*)\b(?!\s*\()/g) {
        $float_name{$1} = 1;
    }

    $source =~ s{
        \b([A-Za-z_][A-Za-z0-9_]*)
        ((?:\s*(?:\.[A-Za-z]+|\[[^\]\r\n]+\]))?)
        (\s*$operator\s*)
        (-?[0-9]+)(?![.A-Za-z0-9_])
    }{$float_name{$1} ? "$1$2$3$4.0" : $&}gex;

    $source =~ s{
        (^|[^A-Za-z0-9_.])
        (-?[0-9]+)
        (\s*$operator\s*)
        ([A-Za-z_][A-Za-z0-9_]*)
        ((?:\s*(?:\.[A-Za-z]+|\[[^\]\r\n]+\]))?)
    }{$float_name{$4} ? "$1$2.0$3$4$5" : $&}gmex;

    $source =~ s/\b(mod|pingPong)\s*\(([^,\r\n]+),\s*(-?[0-9]+)\s*\)/$1($2, $3.0)/g;
    $source =~ s/((?:float|vec[234])\s*\([^;\r\n]+\))\s*([+*\/-])\s*(-?[0-9]+)(?![.A-Za-z0-9_])/$1 $2 $3.0/g;
    return $source;
}

sub convert_to_webgl {
    my ($source) = @_;
    $source =~ s/^\xEF\xBB\xBF//;

    if ($source =~ /#version/) {
        $source =~ s/\A.*?(?=#version)//s;
        $source =~ s/\A#version[^\r\n]*/#version 300 es/;
    } else {
        $source = "#version 300 es\n$source";
    }
    if ($source !~ /\bprecision\s+(?:lowp|mediump|highp)\s+float\s*;/) {
        $source =~ s/(\A#version[^\r\n]*\r?\n)/$1precision highp float;\nprecision highp int;\n/;
    }

    $source =~ s/\bin\s+vec2\s+tc\s*;/in vec2 TexCoord;\n#define tc TexCoord/g;
    $source =~ s/\btexture2D\s*\(/texture(/g;
    $source =~ s/\btexture1D\s*\(/texture(/g;
    $source =~ s/\buniform\s+(float|vec[234])\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;]+);/const $1 $2 = $3;/g;

    if ($source =~ /\bgl_FragColor\b/) {
        $source =~ s/(\A#version[^\r\n]*\r?\n)/$1out vec4 mxFragColor;\n/;
        $source =~ s/\bgl_FragColor\b/mxFragColor/g;
    }
    $source = make_fragment_output_safe($source);
    $source = normalize_float_literals($source);
    return $source;
}

make_path($cache_dir);
my @filenames = grep { length && substr($_, 0, 1) ne '#' }
    map { s/^\s+|\s+$//gr } split /\r?\n/, read_file($source_index);

my @cached;
my $written = 0;
my $unchanged = 0;
my $skipped = 0;
for my $filename (@filenames) {
    die "Unsafe shader filename in index: $filename\n"
        if $filename =~ m{(?:^|/)\.\.(?:/|$)} || File::Spec->file_name_is_absolute($filename);
    my $source_path = File::Spec->catfile($shader_dir, $filename);
    die "Shader listed in index does not exist: $source_path\n" unless -f $source_path;
    my $source = read_file($source_path);
    if (uses_unsupported_inputs($filename, $source)) {
        ++$skipped;
        next;
    }

    my $destination = File::Spec->catfile($cache_dir, $filename);
    my (undef, $directories) = File::Spec->splitpath($destination);
    make_path($directories) if length $directories;
    if (write_if_changed($destination, convert_to_webgl($source))) {
        ++$written;
    } else {
        ++$unchanged;
    }
    push @cached, $filename;
}

write_if_changed($cache_index, join('', map { "$_\n" } @cached));
utime undef, undef, $cache_index;
print "WebGL shader cache: ", scalar(@cached), " cached, $written updated, $unchanged unchanged, $skipped skipped\n";
