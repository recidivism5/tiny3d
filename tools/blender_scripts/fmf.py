#Spec:



bl_info = {
    "name": "Fractured Mesh Format Exporter",
    "description": "Exports a mesh to fmf format. Adapted from Mark Kughler's (GoliathForgeOnline) GEO format export script: https://www.gamedev.net/blogs/entry/2266917-blender-mesh-export-script/",
    "author": "Ian Bryant",
    "version": (1, 0),
    "blender": (2, 79, 0),
    "location": "File > Export > fmf",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"
}

import math
import os
import bpy
import bmesh
import struct
from bpy import context

def getTextureName(material):
    nodes = material.node_tree.nodes
    glossy = next(n for n in nodes if n.type == "BSDF_GLOSSY")
    glass = next(n for n in nodes if n.type == "BSDF_GLASS")
    if glossy:
        return glossy.inputs["Color"].links[0].from_node.image.name
    elif glass:
        return glass.inputs["Color"].links[0].from_node.image.name

def triangulate_object(obj):
    me = obj.data
    # Get a BMesh representation
    bm = bmesh.new()
    bm.from_mesh(me)

    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    # V2.79 : bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method=0, ngon_method=0)

    # Finish up, write the bmesh back to the mesh
    bm.to_mesh(me)
    bm.free()

class ObjectExport(bpy.types.Operator):
    bl_idname = "object.export_fmf"
    bl_label = "Fractured Mesh Format Export"
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".fmf"
    
    total           = bpy.props.IntProperty(name="Steps", default=2, min=1, max=100)
    filter_glob     = bpy.props.StringProperty(default="*.fmf", options={'HIDDEN'}, maxlen=255)
    filepath        = bpy.props.StringProperty(subtype='FILE_PATH')
    
    def execute(self, context):
        if(context.active_object.mode == 'EDIT'):
            bpy.ops.object.mode_set(mode='OBJECT')

        with open(self.filepath, "wb") as f:

            vcount = 0
            for obj in bpy.data.objects:
                triangulate_object(obj)
                vcount += 3*len(obj.data.polygons)
            f.write(struct.pack("<i",vcount)) #vertex count
            
            for obj in bpy.data.objects:
                uv_layer = obj.data.uv_layers.active.data

                polygonGroups = []
                for slot in obj.material_slots:
                    polygonGroups.append([])
                for p in obj.data.polygons:
                    polygonGroups[p.material_index].append(p)

                for group in polygonGroups:
                    for p in group:
                        positions = []
                        for i in range(3):
                            pi = p.vertices[i]
                            v = obj.data.vertices[pi].co
                            positions.append([v.y, v.z, v.x])
                        a = [positions[1][0]-positions[0][0],positions[1][1]-positions[0][1],positions[1][2]-positions[0][2]]
                        b = [positions[2][0]-positions[0][0],positions[2][1]-positions[0][1],positions[2][2]-positions[0][2]]
                        normal = [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
                        mag = math.sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2])
                        for i in range(3):
                            normal[i] /= mag

                        for i in range(3):
                            pi = p.vertices[i]
                            uvi = p.loop_indices[i]
                            v = obj.data.vertices[pi].co
                            n = obj.data.vertices[pi].normal
                            uv = uv_layer[uvi].uv
                            f.write(struct.pack("<3f", v.y, v.z, v.x)) #position
                            f.write(struct.pack("<3f", n.y, n.z, n.x)) #normal
                            #f.write(struct.pack("<3f", normal[0], normal[1], normal[2])) #normal
                            f.write(struct.pack("<2f", uv.x, uv.y)) #uv
            
            obj = bpy.data.objects[0]
            f.write(struct.pack("<i", len(obj.material_slots))) #material count
            for i in range(len(obj.material_slots)):
                slot = obj.material_slots[i]
                nodes = slot.material.node_tree.nodes
                
                bsdf = 0
                for n in nodes:
                    if n.type == "BSDF_DIFFUSE":
                        bsdf = n
                        break
                    elif n.type == "BSDF_GLOSSY":
                        bsdf = n
                        break
                    elif n.type == "BSDF_GLASS":
                        bsdf = n
                        break

                texName = bsdf.inputs["Color"].links[0].from_node.image.name
                tex = bpy.data.images[texName]
                texPath = os.path.normpath(bpy.path.abspath(tex.filepath, library=tex.library))
                with open(texPath,"rb") as texFile:
                    texBytes = texFile.read()
                    f.write(struct.pack("<i",len(texBytes))) #texture len
                    f.write(texBytes) #texture
            
            f.write(struct.pack("<i",len(bpy.data.objects))) #object count
            for obj in bpy.data.objects:
                loc = obj.location
                f.write(struct.pack("<3f", loc.y, loc.z, loc.x)) #position

                polygonGroups = []
                for slot in obj.material_slots:
                    polygonGroups.append([])
                for p in obj.data.polygons:
                    polygonGroups[p.material_index].append(p)

                for group in polygonGroups:
                    f.write(struct.pack("<i",3*len(group))) #vertex count per material

        return {'FINISHED'}

    def invoke(self, context, event):
        filename = bpy.path.basename(bpy.data.filepath)
        filename = os.path.splitext(filename)[0]
        self.filepath = filename + ".fmf"
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}



# Add trigger into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ObjectExport.bl_idname, text="Fractured Mesh Format (.fmf)")
    

def register():
    bpy.utils.register_class(ObjectExport)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ObjectExport)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()