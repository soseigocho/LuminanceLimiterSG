local ffi = require("ffi")
pcall(ffi.cdef, [[
    typedef struct RGBA_ {
        uint8_t r,g,b,a;
    } RGBA;
]])

pcall(ffi.cdef, [[
    typedef struct Pixel_YC_ {
        int16_t y,cb,cr
    } Pixel_YC;
]])

local ll_mod = {}

ll_mod.eps = 2.220 * 10^-16
ll_mod.lmax = 4096.0
ll_mod.lmin = 0.0

local bit = require("bit")
local rshift = bit.rshift

ll_mod.pixel_yc = function(rgba)
    y  = rshift(4918*rgba.r+354, 10) +
        rshift(9655*rgba.g+585, 10) +
        rshift(1875*rgba.b+523, 10)
    cb = rshift(-2775*rgba.r+240, 10) +
        rshift(-5449*rgba.g+515, 10) +
        rshift( 8224*rgba.b+256, 10)
    cr = rshift(8224*rgba.r+256, 10) +
        rshift(-6887*rgba.g+110, 10) +
        rshift(-1337*rgba.b+646, 10)
    return y, cb, cr
end

ll_mod.Character = {}
ll_mod.Character.new = function(
    interpolation_mode,
    top_limit, top_threashold,
    bottom_limit, bottom_threashold,
    top_peak, bottom_peak)

    if interpolation_mode == 0 then
        local top_ratio
        if top_peak >= top_limit then
            top_a = (top_limit - top_threashold) /
                (top_peak - top_threashold)
            top_b = top_limit - top_peak * top_a
        else
            top_ratio = 1.0
        end

        local bottom_ratio
        if bottom_peak <= bottom_limit then
            bottom_a = (bottom_threashold - bottom_limit) /
                (bottom_threashold - bottom_peak)
            bottom_b = bottom_limit - bottom_peak * bottom_a
        else
            bottom_ratio = 1.0
        end

        return function(value)
            if value >= top_threashold then
                return value * top_a + top_b
            elseif value  < top_threashold and value >= bottom_threashold then
                return value
            else
                return value * bottom_a + bottom_b
            end
        end
    end
    return nil
end

return ll_mod
