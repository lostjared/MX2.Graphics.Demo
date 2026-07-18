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

sub add_common_adjustments {
    my ($filename, $source) = @_;

    my ($output_name) = $source =~ /\bout\s+vec4\s+([A-Za-z_][A-Za-z0-9_]*)\s*;/;
    die "Unable to find fragment output while caching $filename\n" unless defined $output_name;
    die "Unable to find texture coordinate input while caching $filename\n"
        unless $source =~ /\bin\s+vec2\s+TexCoord\s*;/;
    die "Unable to find primary sampler while caching $filename\n"
        unless $source =~ /\buniform\s+sampler2D\s+samp\s*;/;

    my @uniform_names = qw(
        iSpeed iAmplitude iFrequency iBrightness iContrast iSaturation
        iHueShift iZoom iRotation iQuality iDebugMode
    );
    my %native_uniform = map {
        $_ => ($source =~ /\buniform\s+float\s+[^;]*\b\Q$_\E\b[^;]*;/ ? 1 : 0)
    } @uniform_names;
    my @missing_uniforms = grep {
        !$native_uniform{$_}
    } @uniform_names;
    my $uniforms = join '', map { "uniform float $_;\n" } @missing_uniforms;

    my $frequency = $native_uniform{iFrequency} ? '1.0' : 'iFrequency';
    my $zoom = $native_uniform{iZoom} ? '1.0' : 'iZoom';
    my $rotation = $native_uniform{iRotation} ? '0.0' : 'iRotation';
    my $quality = $native_uniform{iQuality} ? '1.0' : 'iQuality';
    my $brightness = $native_uniform{iBrightness} ? '1.0' : 'iBrightness';
    my $contrast = $native_uniform{iContrast} ? '1.0' : 'iContrast';
    my $saturation = $native_uniform{iSaturation} ? '1.0' : 'iSaturation';
    my $hue_shift = $native_uniform{iHueShift} ? '0.0' : 'iHueShift';
    my $amplitude_adjustment = $native_uniform{iAmplitude}
        ? ''
        : '    mxCacheEffectColor = mix(mxCacheInputColor, mxCacheEffectColor, iAmplitude);' . "\n";
    my $debug_adjustment = $native_uniform{iDebugMode}
        ? ''
        : <<'GLSL';
    if (iDebugMode > 0.5 && TexCoord.x < 0.5) {
        mxCacheEffectColor = mxCacheInputColor;
    }
GLSL

    my $helpers = <<'GLSL';

vec2 mxCacheApplyCoordinateAdjustments(vec2 uv, float frequency, float zoom,
                                       float rotation, float quality, vec2 resolution) {
    vec2 p = uv - vec2(0.5);
    float c = cos(rotation);
    float s = sin(rotation);
    p = mat2(c, -s, s, c) * p;
    p *= max(frequency, 0.0);
    p /= max(abs(zoom), 0.001);
    uv = p + vec2(0.5);
    if (quality < 1.0) {
        vec2 grid = max(resolution * max(quality, 0.05), vec2(1.0));
        uv = (floor(uv * grid) + vec2(0.5)) / grid;
    }
    return uv;
}

vec3 mxCacheRotateHue(vec3 col, float angle) {
    float U = cos(angle);
    float W = sin(angle);
    mat3 R = mat3(
        0.299 + 0.701*U + 0.168*W,
        0.587 - 0.587*U + 0.330*W,
        0.114 - 0.114*U - 0.497*W,
        0.299 - 0.299*U - 0.328*W,
        0.587 + 0.413*U + 0.035*W,
        0.114 - 0.114*U + 0.292*W,
        0.299 - 0.300*U + 1.250*W,
        0.587 - 0.588*U - 1.050*W,
        0.114 + 0.886*U - 0.203*W
    );
    return clamp(R * col, 0.0, 1.0);
}

vec3 mxCacheApplyColorAdjustments(vec3 col, float brightness, float contrast,
                                  float saturation, float hueShift) {
    col *= brightness;
    col = (col - 0.5) * contrast + 0.5;
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, saturation);
    return mxCacheRotateHue(col, hueShift);
}

vec2 mxCacheTexCoord;
GLSL

    my $precision_end;
    while ($source =~ /\bprecision\s+(?:lowp|mediump|highp)\s+(?:float|int)\s*;/g) {
        $precision_end = $+[0];
    }
    die "Unable to find precision declarations while caching $filename\n"
        unless defined $precision_end;
    substr($source, $precision_end, 0, "\n$uniforms$helpers");
    $source =~ s/#define\s+tc\s+TexCoord/#define tc mxCacheTexCoord/;

    my $renamed = ($source =~ s/\bvoid\s+main\s*\(\s*(?:void)?\s*\)/void mxCacheShaderMain()/);
    die "Unable to wrap fragment main while caching $filename\n" unless $renamed;

    $source .= <<"GLSL";

void main() {
    mxCacheTexCoord = mxCacheApplyCoordinateAdjustments(
        TexCoord, $frequency, $zoom, $rotation, $quality, vec2(textureSize(samp, 0)));
    vec4 mxCacheInputColor = texture(samp, mxCacheTexCoord);
    mxCacheShaderMain();
    vec4 mxCacheEffectColor = $output_name;
$amplitude_adjustment    mxCacheEffectColor.rgb = mxCacheApplyColorAdjustments(
        mxCacheEffectColor.rgb, $brightness, $contrast, $saturation, $hue_shift);
$debug_adjustment    $output_name = mxCacheEffectColor;
}
GLSL
    return $source;
}

sub qualify_array_precision {
    my ($source) = @_;
    my $array = qr/\b(?:float|int|uint|vec[234]|ivec[234]|uvec[234]|mat[234](?:x[234])?)\s+[A-Za-z_][A-Za-z0-9_]*\s*\[[^\]\r\n]*\]/;
    my $inserted = 0;

    while ($source =~ /$array/g) {
        my $start = $-[0];
        my $end = $+[0];
        my $before = substr($source, 0, $start);
        next if $before =~ /\b(?:lowp|mediump|highp)\s*\z/;

        substr($source, $start, 0, 'highp ');
        ++$inserted;
        my $line_end = index($source, "\n", $start);
        if ($line_end > 0 && substr($source, $line_end - 1, 1) eq "\r") {
            substr($source, $line_end - 1, 1, '');
        }
        pos($source) = $end + length('highp ');
    }
    return $source;
}

sub convert_to_webgl {
    my ($filename, $source) = @_;
    $source =~ s/^\xEF\xBB\xBF//;
    $source =~ s/\r\n?/\n/g;

    if ($source =~ /#version/) {
        $source =~ s/\A.*?(?=#version)//s;
        $source =~ s/\A#version[^\r\n]*/#version 300 es/;
    } else {
        $source = "#version 300 es\n$source";
    }
    if ($source !~ /\bprecision\s+(?:lowp|mediump|highp)\s+float\s*;/) {
        $source =~ s/(\A#version[^\r\n]*\r?\n)/$1precision highp float;\nprecision highp int;\n/;
    }

    if ($source =~ /\bin\s+vec2\s+tc\s*;/) {
        $source =~ s/\bin\s+vec2\s+tc\s*;/in vec2 TexCoord;\n#define tc TexCoord/g;
    } elsif ($source =~ /\bgl_FragCoord\.xy\s*\/\s*iResolution(?:\.xy)?\b/) {
        $source =~ s/\bgl_FragCoord\.xy\s*\/\s*iResolution(?:\.xy)?\b/tc/g;
        my $precision_end;
        while ($source =~ /\bprecision\s+(?:lowp|mediump|highp)\s+(?:float|int)\s*;/g) {
            $precision_end = $+[0];
        }
        die "Unable to insert texture coordinate input while caching $filename\n"
            unless defined $precision_end;
        substr($source, $precision_end, 0, "\nin vec2 TexCoord;\n#define tc TexCoord");
    }
    $source =~ s/\btexture2D\s*\(/texture(/g;
    $source =~ s/\btexture1D\s*\(/texture(/g;
    my $common_uniform = qr/(?:iSpeed|iAmplitude|iFrequency|iBrightness|iContrast|iSaturation|iHueShift|iZoom|iRotation|iQuality|iDebugMode)/;
    $source =~ s/\buniform\s+float\s+($common_uniform)\s*=\s*[^;]+;/uniform float $1;/g;
    $source =~ s/\bconst\s+float\s+($common_uniform)\s*=\s*[^;]+;/uniform float $1;/g;
    $source =~ s/\buniform\s+(float|vec[234])\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([^;]+);/const $1 $2 = $3;/g;

    if ($source =~ /\bgl_FragColor\b/) {
        $source =~ s/(\A#version[^\r\n]*\r?\n)/$1out vec4 mxFragColor;\n/;
        $source =~ s/\bgl_FragColor\b/mxFragColor/g;
    }
    $source = qualify_array_precision($source);
    $source = make_fragment_output_safe($source);
    $source = normalize_float_literals($source);
    $source = add_common_adjustments($filename, $source);
    $source =~ s/\t/    /g;
    $source =~ s/[ \t]+$//mg;
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
    if (write_if_changed($destination, convert_to_webgl($filename, $source))) {
        ++$written;
    } else {
        ++$unchanged;
    }
    push @cached, $filename;
}

write_if_changed($cache_index, join('', map { "$_\n" } @cached));
utime undef, undef, $cache_index;
print "WebGL shader cache: ", scalar(@cached), " cached, $written updated, $unchanged unchanged, $skipped skipped\n";
