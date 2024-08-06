const std = @import("std");

pub fn build(b: *std.Build) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard optimization options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
    // set a preferred release mode, allowing the user to decide how to optimize.
    const optimize = b.standardOptimizeOption(.{});

    const lib = b.addStaticLibrary(.{
        .name = "libhtw",
        .link_libc = true,
        .target = target,
        .optimize = optimize,
    });
    lib.addCSourceFiles(.{ .root = b.path("src"), .files = &.{
        "htw_core_math.c",
        "htw_random.c",
    } });
    lib.addCSourceFiles(.{ .root = b.path("src/geomap"), .files = &.{
        "htw_geomap_chunkmap.c",
        "htw_geomap_generators.c",
        "htw_geomap_hexgrid.c",
        "htw_geomap_spatialStorage.c",
        "htw_geomap_valuemap.c",
    } });
    lib.addIncludePath(b.path("include"));
    lib.installHeadersDirectory(b.path("include"), "", .{});

    b.installArtifact(lib);
}
