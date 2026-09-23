local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local ViewControl = require "Unit.ViewControl"
local ply = app.SECTION_PLY

local CatControl = Class {}
CatControl:include(ViewControl)

function CatControl:init(args)
  ViewControl.init(self, "photo")
  self:setClassName("CatControl")

  local width = args.width or (2 * ply)

  local graphic = app.Graphic(0, 0, width, 64)
  self.pDisplay = libFestivamente.CatPhoto(0, 0, width, 64)
  local displayGraphic = self.pDisplay:asGraphic()
  graphic:addChild(displayGraphic)
  self:setMainCursorController(displayGraphic)
  self:setControlGraphic(graphic)

  for i = 1, (width // ply) do
    self:addSpotDescriptor {
      center = (i - 0.5) * ply
    }
  end

  self.subGraphic = app.Graphic(0, 0, 128, 64)
end

return CatControl
