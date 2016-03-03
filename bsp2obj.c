/*
 * bsp2obj
 *
 * Copyright (C) 2016 Florian Zwoch <fzwoch@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <string.h>

struct entry_s {
	guint32 offset;
	guint32 size;
};

struct header_s {
	guint32 version;
	struct entry_s entities;
	struct entry_s planes;
	struct entry_s miptex;
	struct entry_s vertices;
	struct entry_s visilist;
	struct entry_s nodes;
	struct entry_s texinfo;
	struct entry_s faces;
	struct entry_s lightmaps;
	struct entry_s clipnodes;
	struct entry_s leaves;
	struct entry_s faces_list;
	struct entry_s edges;
	struct entry_s edges_list;
	struct entry_s models;
};

struct vec3_s {
	gfloat x;
	gfloat y;
	gfloat z;
};

struct boundbox_s {
	struct vec3_s min;
	struct vec3_s max;
};

struct model_s {
	struct boundbox_s bound;
	struct vec3_s origin;
	guint32 node_id0;
	guint32 node_id1;
	guint32 node_id2;
	guint32 node_id3;
	guint32 numleaves;
	guint32 face_id;
	guint32 face_num;
};

struct mipheader_s {
	guint32 numtex;
	guint32 offsets[];
};

struct miptex_s {
	gchar name[16];
	guint32 width;
	guint32 height;
	guint32 offset1;
	guint32 offset2;
	guint32 offset4;
	guint32 offset8;
};

struct surface_s {
	struct vec3_s vectorS;
	gfloat distS;
	struct vec3_s vectorT;
	gfloat distT;
	guint32 texture_id;
	guint32 animated;
};

struct edge_s {
	guint16 vertex0;
	guint16 vertex1;
};

struct face_s {
	guint16 plane_id;
	guint16 side;
	gint32 ledge_id;
	guint16 ledge_num;
	guint16 texinfo_id;
	guint8 typelight;
	guint8 baselight;
	guint8 light[2];
	gint32 lightmap;
};

static guint map_vertex(GHashTable *map, GString *obj, struct vec3_s *vertex, guint vertex_idx)
{
	gpointer value = g_hash_table_lookup(map, GINT_TO_POINTER(vertex_idx));
	
	if (value != NULL)
	{
		return GPOINTER_TO_INT(value);
	}
	
	gint mapped_idx = g_hash_table_size(map) + 1;
	g_hash_table_insert(map, GINT_TO_POINTER(vertex_idx), GINT_TO_POINTER(mapped_idx));
	
	g_string_append_printf(obj, "v %g %g %g\n", vertex[vertex_idx].x, vertex[vertex_idx].y, vertex[vertex_idx].z);
	
	return mapped_idx;
}

int main(int argc, char **argv)
{
	GError *err = NULL;
	gchar *buf = NULL;
	gsize buf_len = 0;
	
	GHashTable *map = g_hash_table_new(g_direct_hash, g_direct_equal);
	GString *obj = g_string_new(NULL);
	
	g_file_get_contents(argv[1], &buf, &buf_len, &err);
	if (err != NULL)
	{
		g_error("%s", err->message);
		g_error_free(err);
		
		return 1;
	}
	
	struct header_s *header = buf;
	struct model_s *models = buf + header->models.offset;
	struct mipheader_s *mipheader = buf + header->miptex.offset;
	struct vec3_s *vertex = buf + header->vertices.offset;
	struct surface_s *surfaces = buf + header->texinfo.offset;
//	gint16 *faces_list = buf + header->faces_list.offset;
	struct face_s *faces = buf + header->faces.offset;
	gint32 *edges_list = buf + header->edges_list.offset;
	struct edge_s *edges = buf + header->edges.offset;
	
	gchar *map_name = g_path_get_basename(argv[1]);
	map_name[strcspn(map_name, ".")] = '\0';
	
	for (gint i = 0; i < mipheader->numtex; i++)
	{
		struct miptex_s *miptex = buf + header->miptex.offset + mipheader->offsets[i];
		
		if (g_strcmp0(miptex->name, "trigger") == 0 || g_strcmp0(miptex->name, "clip") == 0 || g_strcmp0(miptex->name, "black") == 0)
		{
			continue;
		}
		
		if (miptex->name[0] == '*')
		{
			miptex->name[0] = '+';
		}
		
		g_string_append_printf(obj, "newmtl %s\n", miptex->name);
		g_string_append_printf(obj, "Ka 1 1 1\n");
		g_string_append_printf(obj, "Kd 1 1 1\n");
		g_string_append_printf(obj, "Ks 0 0 0\n");
		g_string_append_printf(obj, "Tr 1\n");
		g_string_append_printf(obj, "illum 1\n");
		g_string_append_printf(obj, "Ns 0\n");
		g_string_append_printf(obj, "map_Kd textures/%s.jpg\n", miptex->name);
	}
	
	gchar *out_file = g_strdup_printf("%s.mtl", map_name);
	
	g_file_set_contents(out_file, obj->str, obj->len, &err);
	if (err != NULL)
	{
		g_error("%s", err->message);
		g_error_free(err);
		
		return 1;
	}
	
	g_free(out_file);
	
	for (gint k = 0; k < header->models.size / sizeof(struct model_s); k++)
	{
		GString *obj_uvs = g_string_new(NULL);
		GString *obj_faces = g_string_new(NULL);
		gint count = 1;
		
		g_string_printf(obj, "mtllib %s.mtl\n", map_name);
		
		for (gint i = 0; i < models[k].face_num; i++)
		{
			struct face_s *face = &faces[models[k].face_id + i];
			struct edge_s *edge = edges + ABS(edges_list[face->ledge_id]);
			struct miptex_s *miptex = buf + header->miptex.offset + mipheader->offsets[surfaces[face->texinfo_id].texture_id];
			
			if (g_strcmp0(miptex->name, "trigger") == 0 || g_strcmp0(miptex->name, "clip") == 0 || g_strcmp0(miptex->name, "black") == 0)
			{
				continue;
			}
			
			g_string_append_printf(obj_faces, "usemtl %s\n", miptex->name);
			
			for (gint j = 1; j < face->ledge_num - 1; j++)
			{
				struct edge_s *edge1 = edges + ABS(edges_list[face->ledge_id + j]);
				struct edge_s *edge2 = edges + ABS(edges_list[face->ledge_id + j + 1]);
				
				g_string_append_printf(obj_faces, "f %u/%d %u/%d %u/%d\n",
									   map_vertex(map, obj, vertex, edges_list[face->ledge_id] < 0 ? edge->vertex1 : edge->vertex0),
									   count,
									   map_vertex(map, obj, vertex, edges_list[face->ledge_id + j + 1] < 0 ? edge2->vertex1 : edge2->vertex0),
									   count + 2,
									   map_vertex(map, obj, vertex, edges_list[face->ledge_id + j] < 0 ? edge1->vertex1 : edge1->vertex0),
									   count + 1);
				
				count += 3;
				
				float u[3];
				float v[3];
				
				u[0] = (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].x : vertex[edge->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].y : vertex[edge->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].z : vertex[edge->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
				v[0] = (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].x : vertex[edge->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].y : vertex[edge->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id] < 0 ? vertex[edge->vertex1].z : vertex[edge->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;
				
				u[1] = (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].x : vertex[edge1->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].y : vertex[edge1->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].z : vertex[edge1->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
				v[1] = (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].x : vertex[edge1->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].y : vertex[edge1->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id + j] < 0 ? vertex[edge1->vertex1].z : vertex[edge1->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;
				
				u[2] = (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].x : vertex[edge2->vertex0].x) * surfaces[face->texinfo_id].vectorS.x + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].y : vertex[edge2->vertex0].y) * surfaces[face->texinfo_id].vectorS.y + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].z : vertex[edge2->vertex0].z) * surfaces[face->texinfo_id].vectorS.z + surfaces[face->texinfo_id].distS;
				v[2] = (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].x : vertex[edge2->vertex0].x) * surfaces[face->texinfo_id].vectorT.x + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].y : vertex[edge2->vertex0].y) * surfaces[face->texinfo_id].vectorT.y + (edges_list[face->ledge_id + j + 1] < 0 ? vertex[edge2->vertex1].z : vertex[edge2->vertex0].z) * surfaces[face->texinfo_id].vectorT.z + surfaces[face->texinfo_id].distT;
				
				g_string_append_printf(obj_uvs, "vt %g %g\nvt %g %g\nvt %g %g\n",
									   u[0] / miptex->width,
									   1 - v[0] / miptex->height,
									   u[1] / miptex->width,
									   1 - v[1] / miptex->height,
									   u[2] / miptex->width,
									   1 - v[2] / miptex->height);
			}
		}
		
		obj = g_string_append(obj, obj_uvs->str);
		obj = g_string_append(obj, obj_faces->str);
		
		g_string_free(obj_uvs, TRUE);
		g_string_free(obj_faces, TRUE);
		
		if (g_hash_table_size(map) == 0)
		{
			continue;
		}
		
		g_hash_table_remove_all(map);
		
		if (k == 0)
		{
			out_file = g_strdup_printf("%s.obj", map_name);
		}
		else
		{
			out_file = g_strdup_printf("%s_%d.obj", map_name, k);
		}
		
		g_file_set_contents(out_file, obj->str, obj->len, &err);
		if (err != NULL)
		{
			g_error("%s", err->message);
			g_error_free(err);
			
			return 1;
		}
		
		g_free(out_file);
	}
	
	g_hash_table_unref(map);
	g_string_free(obj, TRUE);
	g_free(buf);
	
	return 0;
}
