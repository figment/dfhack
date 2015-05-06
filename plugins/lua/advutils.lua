-- advutils.lua

local _ENV = mkmodule('plugins.advutils')

--[[

 Native functions:

 * adv_teleport(world.x,world.y,rgn.x,rgn.y)

 Export functions:
 * siteTeleport() - Teleport to selected site
 
--]]

-- teleport to site
--   Shows dialog for selecting a site and then possible offset from site
function siteTeleport() 
  local script=require('gui.script')

  local wd = df.global.world.world_data
  if wd == nil then
    print("World not created")
    return nil
  end
  local reg_x = wd.adv_region_x
  local reg_y = wd.adv_region_y

  -- calculate distance from current location
  local site_dist = function(s)
    return math.sqrt((reg_x-s.pos.x)*(reg_x-s.pos.x) + (reg_y-s.pos.y)*(reg_y-s.pos.y))
  end

  -- sort list of sites by distance from the current location
  local sort_site = function(sites)
    local order = function(a, b)
      if a ~= nil and b ~= nil then
        local d = site_dist(a) - site_dist(b)
        if d ~= 0 then return d < 0 end
        if a.pos.x ~= b.pos.x then return a.pos.x < b.pos.x end
        if a.pos.y ~= b.pos.y then return a.pos.y < b.pos.y end
        if a.rgn_min_x ~= b.rgn_min_x then return a.rgn_min_x < b.rgn_min_x end
        if a.rgn_min_y ~= b.rgn_min_y then return a.rgn_min_y < b.rgn_min_y end
      end
      return 0
    end
    local target = {}
    for _,v in pairs(sites) do target[#target+1] = v end
    table.sort(target, order)
    return target
  end

  script.start(function()
    local gui_sites=require('gui.sites')
    local ok,_,site = gui_sites.showSitePrompt('Select Site', 'Which site to select?', sort_site(wd.sites))
    if ok and site~=nil then
      local directions = {}
      table.insert(directions, { text='None', offset={x= 0,y= 0} })    
      table.insert(directions, { text='N',    offset={x= 0,y=-1} })    
      table.insert(directions, { text='NE',   offset={x= 1,y=-1} })    
      table.insert(directions, { text='E',    offset={x= 1,y= 0} })    
      table.insert(directions, { text='SE',   offset={x= 1,y= 1} })    
      table.insert(directions, { text='S',    offset={x= 0,y= 1} })    
      table.insert(directions, { text='SW',   offset={x=-1,y= 1} })    
      table.insert(directions, { text='W',    offset={x=-1,y= 0} })    
      table.insert(directions, { text='NW',   offset={x=-1,y=-1} })    
      local ok,_,item = script.showListPrompt('Offset', 'Select offset direction', 
        COLOR_LIGHTGREEN, directions )
      if ok and item~=nil then
        local rgn_x = math.max(0,math.floor((site.rgn_max_x + site.rgn_min_x)/2)) + item.offset.x
        local rgn_y = math.max(0,math.floor((site.rgn_max_y + site.rgn_min_y)/2)) + item.offset.y
        _ENV.adv_teleport(site.pos.x,site.pos.y,rgn_x,rgn_y)
      end
    end
  end)
  
end

return _ENV
