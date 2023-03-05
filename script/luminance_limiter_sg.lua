local ll_mod = {}

ll_mod.eps = 2.220 * 10^-16
ll_mod.lmax = 4096.0
ll_mod.lmin = 0.0

ll_mod.id = function(...)
    return ...
end

ll_mod.Pixel_YC = {}
ll_mod.Pixel_YC.new = function(Y, cb, cr, a)
    return {
        Y = Y,
        cb = cb,
        cr = cr,
        a = a,
        luminance = function(self)
            return self.Y / ll_mod.lmax
        end
    }
end

ll_mod.LuminousImage = {}
ll_mod.LuminousImage.new = function(obj, luminous_mode)
    if luminous_mode == 0 then
        obj.pixeloption("type", "yc")
        obj.pixeloption("get", "obj")
        obj.pixeloption("put", "obj")
        return {
            img = obj,
            map = function (self, f)
                    local width, height = self.img.getpixel()
                    for j = 0, height - 1 do
                        for i = 0, width - 1 do
                            local Y, cb, cr, a = self.img.getpixel(i, j, "yc")
                            local pixel = ll_mod.Pixel_YC.new(Y, cb, cr, a)
                            coroutine.yield(f(pixel))
                        end
                    end
                    return nil
                end,
            fold = function (self, bi_op, init)
                local c = coroutine.create(self.map)
                local accum = init
                repeat
                    local st, yc =
                        coroutine.resume(c, self, ll_mod.id)
                    if not st or yc == nil then
                        return accum
                    end
                    accum = bi_op(accum, yc)
                until coroutine.status(c) == "dead"
                return accum
            end,
            replace = function(self, cf, f)
                local c = coroutine.create(cf)
                local width, height = self.img.getpixel()
                local k = 0
                repeat
                    local i = k % width
                    local j = math.floor(k / width)
                    local st, yc = coroutine.resume(c, self, f)
                    if not st or yc == nil then
                        return self.img
                    end
                    self.img.putpixel(i, j, yc.Y, yc.cb, yc.cr, yc.a)
                    k = k + 1
                until coroutine.status(c) == "dead"
                return self.img
            end,
            luminance_bi_op = function(bi_op)
                return function(a, b)
                    return bi_op(a, b:luminance())
                end
            end
        }
    else
        return nil
    end
end

ll_mod.Character = {}
ll_mod.Character.new = function(
    interpolation_mode,
    top_limit, top_threashold,
    bottom_limit, bottom_threashold,
    top_peak, bottom_peak)

    if interpolation_mode == 0 then
        local top_a
        local top_b
        if top_peak >= top_limit then
            top_a = (top_limit - top_threashold) /
                (top_peak - top_threashold)
            top_b = top_limit - top_peak * top_a
        else
            top_a = 1.0
        end

        local bottom_a
        local bottom_b
        if bottom_peak <= bottom_limit then
            bottom_a = (bottom_threashold - bottom_limit) /
                (bottom_threashold - bottom_peak)
            bottom_b = bottom_limit - bottom_peak * bottom_a
        else
            bottom_a = 1.0
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

ll_mod.Limiter = {}
ll_mod.Limiter.new = function(top_peak, bottom_peak, character, gain)
    if gain == 0 then
        return {
            limiter = function(luminous_pixel)
                luminous_pixel.Y =
                    character(luminous_pixel:luminance()) * ll_mod.lmax
                    return luminous_pixel
            end
        }
    elseif gain > 0 then
        local pr = top_peak / (top_peak - gain)
        return {
            limiter = function(luminous_pixel)
                local gained = luminous_pixel:luminance() * pr
                luminous_pixel.Y = character(gained) * ll_mod.lmax
                return luminous_pixel
            end
        }
    else
        local pr = (bottom_peak - top_peak) / ((bottom_peak - gain) - top_peak)
        return {
            limiter = function(luminous_pixel)
                local gained =
                    (luminous_pixel:luminance() - top_peak) * pr + top_peak
                luminous_pixel.Y = character(gained) * ll_mod.lmax
                return luminous_pixel
            end
        }
    end
end

return ll_mod
