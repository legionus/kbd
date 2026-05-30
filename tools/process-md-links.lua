local process_pages = {
	["howto-compilation.md"] = "howto-compilation.html",
	["howto-contribute.md"] = "howto-contribute.html",
	["howto-pull-request.md"] = "howto-pull-request.html",
	["programming-language.md"] = "programming-language.html",
	["README.md"] = "index.html",
}

local function has_scheme(target)
	return target:match("^[%a][%w+.-]*:") ~= nil
end

local function split_anchor(target)
	local path, anchor = target:match("^([^#]*)(#.*)$")
	if path then
		return path, anchor
	end

	return target, ""
end

function Link(link)
	local target = link.target

	if has_scheme(target) then
		return link
	end

	local path, anchor = split_anchor(target)
	local name = path:match("([^/]+)$")

	if process_pages[name] and not path:match("^%.%./%.%./") then
		link.target = process_pages[name] .. anchor
		return link
	end

	if path == "../../README.md" then
		link.target = "https://github.com/legionus/kbd/blob/master/README.md" .. anchor
		return link
	end

	if path:match("%.md$") then
		path = path:gsub("^%.%./%.%./", "")
		link.target = "https://github.com/legionus/kbd/blob/master/" .. path .. anchor
	end

	return link
end
