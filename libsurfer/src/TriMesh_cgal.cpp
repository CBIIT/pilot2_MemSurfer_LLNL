/**
Copyright (c) 2019, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
Written by Harsh Bhatia (hbhatia@llnl.gov) and Peer-Timo Bremer (bremer5@llnl.gov)
LLNL-CODE-763493. All rights reserved.

This file is part of MemSurfer, Version 1.0.
Released under GNU General Public License 3.0.
For details, see https://github.com/LLNL/MemSurfer.
*/

/// ----------------------------------------------------------------------------
/// ----------------------------------------------------------------------------

#include <map>
#include<string>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <tuple>

#include "TriMesh.hpp"

#include <CGAL/basic.h>
#include <CGAL/iterator.h>

//! -----------------------------------------------------------------------------
//! create a TriMesh object from CGAL Polyhedron
//! -----------------------------------------------------------------------------

TriMesh::TriMesh(const Polyhedron &P) {

    this->mDim = 3;
    this->mName = "TriMesh";

    mVertices.resize(P.size_of_vertices());
    mFaces.resize(P.size_of_facets());

    size_t i = 0;
    for(Polyhedron::Vertex_const_iterator pviter = P.vertices_begin(); pviter != P.vertices_end(); ++pviter, ++i) {
        auto &p = pviter->point();
        mVertices[i] = Vertex(p[0], p[1], p[2]);
    }

    CGAL::Inverse_index<Polyhedron::Vertex_const_iterator> index(P.vertices_begin(), P.vertices_end());

    i = 0;
    for(Polyhedron::Facet_const_iterator pfiter = P.facets_begin(); pfiter != P.facets_end(); ++pfiter, ++i) {

        Polyhedron::Halfedge_around_facet_const_circulator hc = pfiter->facet_begin();

        CGAL_assertion(3 == circulator_size(hc));

        mFaces[i][0] = index[Polyhedron::Vertex_const_iterator(hc->vertex())];    ++hc;
        mFaces[i][1] = index[Polyhedron::Vertex_const_iterator(hc->vertex())];    ++hc;
        mFaces[i][2] = index[Polyhedron::Vertex_const_iterator(hc->vertex())];
    }
}

//! -----------------------------------------------------------------------------
//! spatial sorting of points
//! -----------------------------------------------------------------------------

#ifdef CGAL_AVAILABLE
#include <CGAL/spatial_sort.h>
#include <CGAL/Spatial_sort_traits_adapter_3.h>
#endif

void TriMesh::sort_vertices(std::vector<Point_with_idx> &svertices) const {

#ifndef CGAL_AVAILABLE
    std::cerr << " ERROR: TriMesh::sort_vertices - CGAL not available! cannot sort vertices!\n";s
#else
    typedef CGAL::Spatial_sort_traits_adapter_3<Kernel, CGAL::First_of_pair_property_map<Point_with_idx>> Sort_traits;

    // create a spatially sorted list of points
    svertices.clear();
    svertices.resize(mVertices.size());
    for(size_t i = 0; i < mVertices.size(); i++) {
        svertices[i] = std::make_pair(Point3(mVertices[i][0], mVertices[i][1], mVertices[i][2]), i);
    }

    Sort_traits traits;
    CGAL::spatial_sort(svertices.begin(), svertices.end(), traits);
#endif
}

//! -----------------------------------------------------------------------------
//! Delaunay triangulation
//! -----------------------------------------------------------------------------

#ifdef CGAL_AVAILABLE
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_data_structure_2.h>

#include <CGAL/Delaunay_triangulation_2.h>
#endif

std::vector<TypeIndexI> TriMesh::delaunay(bool verbose) {

#ifndef CGAL_AVAILABLE
    std::cerr << " ERROR: TriMesh::delaunay - CGAL not available! cannot compute Delaunay triangulation!\n";
    return std::vector<TypeIndex>();
#else

    if (this->mDim != 2) {
        std::ostringstream errMsg;
        errMsg << " TriMesh::delaunay() requires a mesh in 2D!" << std::endl;
        throw std::invalid_argument(errMsg.str());
    }

    //typedef CGAL::Projection_traits_xy_3<Kernel> dTraits;
    typedef Kernel dTraits;
    typedef CGAL::Triangulation_vertex_base_with_info_2<TypeIndex, dTraits> Vb_with_idx;
    typedef CGAL::Triangulation_face_base_2<dTraits> Fb;
    typedef CGAL::Triangulation_data_structure_2<Vb_with_idx, Fb> Tds;
    typedef CGAL::Delaunay_triangulation_2<dTraits, Tds> Delaunay;

    if (verbose) {
        std::cout << "   > TriMesh::delaunay()...";
        fflush(stdout);
    }

    // sort the vertices
    std::vector<Point_with_idx> cgalVertices;
    this->sort_vertices(cgalVertices);

    // compute delaunay
    Delaunay dt;
    for(size_t i = 0; i < cgalVertices.size(); i++) {
        Kernel::Point_3 &p = cgalVertices[i].first;
        //Delaunay::Vertex_handle vh = dt.insert(cgalVertices[i].first);
        Delaunay::Vertex_handle vh = dt.insert(Delaunay::Point(p[0],p[1]));
        vh->info() = cgalVertices[i].second;
    }

    mFaces.clear();
    for(auto iter = dt.finite_faces_begin(); iter != dt.finite_faces_end(); ++iter) {
        mFaces.push_back(Face(iter->vertex(0)->info(), iter->vertex(1)->info(), iter->vertex(2)->info()));
    }

    if (verbose)
        std::cout << " Done! created " << dt.number_of_faces() << " triangles using " << dt.number_of_vertices() << " vertices!\n";

    return get_faces();
#endif
}

#include <CGAL/Periodic_2_Delaunay_triangulation_2.h>
#include <CGAL/Periodic_2_Delaunay_triangulation_traits_2.h>

std::vector<TypeIndexI> TriMeshPeriodic::delaunay(bool verbose) {

#ifndef CGAL_AVAILABLE
    std::cerr << " ERROR: TriMeshPeriodic::delaunay - CGAL not available! cannot compute Delaunay triangulation!\n";
    return std::vector<TypeIndexI>();
#else

    if (this->mDim != 2) {
        std::ostringstream errMsg;
        errMsg << " TriMeshPeriodic::delaunay() requires a mesh in 2D!" << std::endl;
        throw std::invalid_argument(errMsg.str());
    }

    typedef CGAL::Periodic_2_Delaunay_triangulation_traits_2<Kernel> dTraits;
    typedef CGAL::Periodic_2_triangulation_vertex_base_2<dTraits> Vb;
    typedef CGAL::Periodic_2_triangulation_face_base_2<dTraits> Fb;

    typedef CGAL::Triangulation_vertex_base_with_info_2<TypeIndex, dTraits, Vb> Vb_with_idx;
    typedef CGAL::Triangulation_data_structure_2<Vb_with_idx, Fb> Tds;
    typedef CGAL::Periodic_2_Delaunay_triangulation_2<dTraits, Tds> Delaunay;

    if (verbose) {
        std::cout << "   > TriMeshPeriodic::delaunay()...";
        fflush(stdout);
    }

    this->wrap_vertices();

    // sort the vertices
    std::vector<Point_with_idx> cgalVertices;
    this->sort_vertices(cgalVertices);

    // compute delaunay
    Delaunay::Iso_rectangle pbox(mBox0[0], mBox0[1], mBox1[0], mBox1[1]);
    Delaunay dt (pbox);

    for(size_t i = 0; i < cgalVertices.size(); i++) {
        Kernel::Point_3 &p = cgalVertices[i].first;
        Delaunay::Vertex_handle vh = dt.insert(Delaunay::Point(p[0],p[1]));
        vh->info() = cgalVertices[i].second;
    }

    mFaces.clear();

    dt.convert_to_9_sheeted_covering();

    const size_t nOrigVerts = mVertices.size();

    Delaunay::Vertex_handle fverts[3];
    Delaunay::Periodic_point fppts[3];
    std::map<Delaunay::Vertex_handle, size_t> duplicateHandles;

    for(auto iter = dt.finite_faces_begin(); iter != dt.finite_faces_end(); ++iter) {

        // get the type of face (number of original vertex in the face)
        uint8_t num_orig_verts = 0;
        for(uint8_t vid = 0; vid < 3; vid++){

            fverts[vid] = iter->vertex(vid);
            fppts[vid] = dt.periodic_point(fverts[vid]);
            const Delaunay::Offset &off = fppts[vid].second;
            if (off[0] == 0 && off[1] == 0)
                num_orig_verts++;
        }

        // ignore the ones that are completely outside
        if (num_orig_verts == 0)
            continue;

        Face face(fverts[0]->info(), fverts[1]->info(), fverts[2]->info());

        // store the ones that are completely inside
        if (num_orig_verts == 3){
            mFaces.push_back(face);
            continue;
        }

        // add this face as is, in the list of periodic faces
        mPeriodicFaces.push_back(face);

        // unwrap the periodic faces
        for(uint8_t vid = 0; vid < 3; vid++){

            const Delaunay::Offset &off = fppts[vid].second;

            // this vertex does not need to be duplicated!
            if (off[0] == 0 && off[1] == 0) {
                continue;
            }

            const Delaunay::Vertex_handle &vh = fverts[vid];
            const size_t origId = vh->info();

            // if this duplicate vertex has not already been added, create it
            if (duplicateHandles.find(vh) == duplicateHandles.end()){

                int off1 = off[0] == 2 ? -1 : int(off[0]);
                int off2 = off[1] == 2 ? -1 : int(off[1]);

                mDuplicateVerts_OrigIds.push_back(dupMap(origId, off1, off2));
                duplicateHandles[vh] = mDuplicateVerts_OrigIds.size() + nOrigVerts - 1;
            }

            // replace the original with duplicate id
            face[vid] = duplicateHandles[vh];
        }
        mTrimmedFaces.push_back(face);
    }

    if (verbose){
        std::cout << " Done! created [" << mFaces.size() << ", " << mPeriodicFaces.size() << ", " << mTrimmedFaces.size() << "] triangles!\n";
    }

    create_duplicate_vertices(verbose);
    return get_faces();
#endif
}

void TriMeshPeriodic::create_duplicate_vertices(bool verbose) {

    size_t norig = mVertices.size();
    size_t ndups = mDuplicateVerts_OrigIds.size();

    if (norig == 0 || ndups == 0 || !bbox_valid)
        return;

    mDuplicateVerts.clear();
    mDuplicateVerts.resize(ndups);

    const Vertex boxw = mBox1 - mBox0;

    for(size_t i = 0; i < ndups; i++) {

        const dupMap &dupMap = mDuplicateVerts_OrigIds[i];

        Vertex dv (mVertices[std::get<0>(dupMap)]);
        int off1 =std::get<1>(dupMap);
        int off2 =std::get<2>(dupMap);

        dv[0] += (off1 == 0) ? 0.0 : float(off1) *boxw[0];
        dv[1] += (off2 == 0) ? 0.0 : float(off2) *boxw[1];
        mDuplicateVerts[i] = dv;
    }

    if (verbose)
        std::cout << "   > TriMeshPeriodic duplicated " << mDuplicateVerts.size() << " vertices!\n";
}

//! -----------------------------------------------------------------------------
//! projection of points on the surface
//! -----------------------------------------------------------------------------

#ifdef CGAL_AVAILABLE
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <CGAL/Projection_traits_xy_3.h>

#endif

std::vector<TypeFunction> TriMesh::project_on_surface(const std::vector<TypeFunction> &points, bool verbose) {

#ifndef CGAL_AVAILABLE
    std::cerr << " ERROR: TriMesh::project_on_surface - CGAL not available! cannot project points on the surface!\n";
    return std::vector<TypeFunction>();
#else

    if (this->mDim != 3) {
        std::ostringstream errMsg;
        errMsg << " TriMesh::project_on_surface() requires a mesh in 3D!" << std::endl;
        throw std::invalid_argument(errMsg.str());
    }
    if (points.size() % 3 != 0) {
        std::ostringstream errMsg;
        errMsg << " TriMesh::project_on_surface() recieved invalid number of 3D points! got " << points.size() << " coordinates!" << std::endl;
        throw std::invalid_argument(errMsg.str());
    }

    typedef std::list<Triangle3>::iterator Iterator;
    typedef CGAL::AABB_triangle_primitive<Kernel, Iterator> Primitive;
    typedef CGAL::AABB_traits<Kernel, Primitive> AABB_triangle_traits;
    typedef CGAL::AABB_tree<AABB_triangle_traits> Tree;

    if (verbose) {
        std::cout << "   > TriMesh::project_on_surface(<" << points.size() << ">)...";
        fflush(stdout);
    }

    // create CGAL vertices
    std::vector<Point3> cgalVertices;
    size_t nverts = mVertices.size();
    cgalVertices.resize(nverts);
    for(size_t i = 0; i < nverts; i++) {
        cgalVertices[i] = Point3(mVertices[i][0], mVertices[i][1], mVertices[i][2]);
    }

    // create CGAL triangles in 3D
    std::list<Triangle3> cgalTriangles;
    size_t nfaces = mFaces.size();
    for(size_t i = 0; i < nfaces; i++) {
        const Face &face = mFaces[i];
        cgalTriangles.push_back(Triangle3(cgalVertices[face[0]], cgalVertices[face[1]], cgalVertices[face[2]]));
    }

    // create CGAL points
    std::vector<Point3> cgalPoints;
    size_t npoints = points.size() / 3;
    cgalPoints.resize(npoints);
    for(size_t i = 0; i < npoints; i++) {
        cgalPoints[i] = Point3(points[3*i], points[3*i+1], points[3*i+2]);
    }

    // create a search datastructure
    Tree tree(cgalTriangles.begin(), cgalTriangles.end());
    tree.accelerate_distance_queries();

    // project the requested points
    std::vector<TypeFunction> rval(4*npoints);
    for(size_t i = 0; i < npoints; i++){

        Tree::Point_and_primitive_id closest =  tree.closest_point_and_primitive(cgalPoints[i]);

        int closest_fid = std::distance(cgalTriangles.begin(), closest.second);
        const Face &cface = mFaces[closest_fid];

        Point3 closest_p = closest.first;
        Point3 bary = TriMesh::Point2Bary(closest_p, cgalVertices[cface[0]], cgalVertices[cface[1]], cgalVertices[cface[2]]);

        rval[4*i] = TypeFunction(closest_fid);
        for(uint8_t j = 0; j < 3; j++)
            rval[4*i+1 + j] = bary[j];
    }

    if (verbose) {
        std::cout << " Done!\n";
    }

    return rval;
#endif
}

//! -----------------------------------------------------------------------------
//! parameterize_xy the vertices on xy-plane (simply ignore the z coordinate)
//! -----------------------------------------------------------------------------

std::vector<TypeFunction> TriMesh::parameterize_xy(bool verbose) {

    if (verbose) {
        std::cout << "   > TriMesh::parameterize_xy(<" << mVertices.size() << ">)...";
        fflush(stdout);
    }

    const size_t nverts = mVertices.size();
    std::vector<TypeFunction> retval(2*nverts);
    for(size_t i = 0; i < nverts; i++) {
        retval[2*i  ] = mVertices[i][0];
        retval[2*i+1] = mVertices[i][1];
    }

    if (verbose) {
        std::cout << " Done!\n";
    }
    return retval;
}


//! -----------------------------------------------------------------------------
//! surface parameterization and meshing
//! -----------------------------------------------------------------------------
#ifdef CGAL_AVAILABLE
#include <CGAL/Surface_mesh.h>
#include <CGAL/Surface_mesh_parameterization/Square_border_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Discrete_conformal_map_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Discrete_authalic_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Mean_value_coordinates_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/ARAP_parameterizer_3.h>
#include <CGAL/Surface_mesh_parameterization/Error_code.h>
#include <CGAL/Surface_mesh_parameterization/parameterize.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <boost/function_output_iterator.hpp>

typedef CGAL::Surface_mesh<Kernel::Point_3>                     SurfaceMesh;

typedef boost::graph_traits<SurfaceMesh>::vertex_descriptor     vertex_descriptor;
typedef boost::graph_traits<SurfaceMesh>::face_descriptor       face_descriptor;
typedef boost::graph_traits<SurfaceMesh>::edge_descriptor       edge_descriptor;
typedef boost::graph_traits<SurfaceMesh>::halfedge_descriptor   halfedge_descriptor;

typedef SurfaceMesh::Property_map<vertex_descriptor, Kernel::Point_2>  UV_pmap;

namespace SMP = CGAL::Surface_mesh_parameterization;
namespace PMP = CGAL::Polygon_mesh_processing;

//! -----------------------------------------------------------------------------
//! convert to and from a CGAL surface mesh object
//! -----------------------------------------------------------------------------
SurfaceMesh to_cgal_mesh(const std::vector<Vertex> &vertices,
                         const std::vector<Face> &faces) {

    SurfaceMesh mesh;

    const size_t nverts = vertices.size();
    const size_t nfaces = faces.size();

    std::vector<vertex_descriptor> smvd (nverts);
    for(size_t i = 0; i < nverts; i++) {
        const Vertex &mv = vertices[i];
        smvd[i] = mesh.add_vertex(Kernel::Point_3(mv[0], mv[1], mv[2]));
    }
    for(size_t i = 0; i < nfaces; i++) {
        const Face &mf = faces[i];
        mesh.add_face(smvd[mf[0]], smvd[mf[1]], smvd[mf[2]]);
    }
    return mesh;
}

void from_cgal_mesh(const SurfaceMesh &mesh, std::vector<Vertex> &vertices,
                                             std::vector<Face> &faces) {

    vertices.clear();
    faces.clear();
    vertices.resize(mesh.number_of_vertices());
    faces.resize(mesh.number_of_faces());

    std::vector<size_t> reindex;
    reindex.resize(mesh.num_vertices());

    size_t vidx = 0;
    BOOST_FOREACH(SurfaceMesh::Vertex_index v, mesh.vertices()){
        auto p = mesh.point(v);
        vertices[vidx] = Vertex(p[0], p[1], p[2]);
        reindex[v] = vidx++;
        //std::cout << v << " : " << (vidx-1) << " : "<< p << std::endl;
    }

    size_t fidx = 0;
    BOOST_FOREACH(SurfaceMesh::Face_index f, mesh.faces()){
        size_t fvidx = 0;
        BOOST_FOREACH(SurfaceMesh::Vertex_index v, CGAL::vertices_around_face(mesh.halfedge(f), mesh)){
            faces[fidx][fvidx++] = reindex[v];
            //std::cout << "(" << v << " : " << reindex[v] << ") ";
        }
        fidx++;
        //std::cout << "\n";
    }
}
#endif

//! -----------------------------------------------------------------------------
//! custom border parameterization
    //! instead of mapping the border to a unit square or a unit circle (what CGAL offers)
    //! map it to its xy projection
//! -----------------------------------------------------------------------------

#ifdef CGAL_AVAILABLE
template<class TriangleMesh>
class Square_border_projection_parameterizer_3
    : public SMP::Square_border_parameterizer_3<TriangleMesh>
{

// Private types
private:
    typedef SMP::Square_border_parameterizer_3<TriangleMesh>   Base;

    typedef typename Base::NT                                   NT;
    typedef typename Base::Offset_map                           Offset_map;

public:
    virtual ~Square_border_projection_parameterizer_3(){}

    /// Constructor.
    Square_border_projection_parameterizer_3() : Base() { }

// Protected operations
protected:

    /// Compute the length of an edge.
    //  Harsh: don't really need this function, except that this is pure virtual
    virtual NT compute_edge_length(const TriangleMesh&  /* mesh */,
                                   vertex_descriptor    /* source */,
                                   vertex_descriptor    /* target */) const
    {
        /// Uniform border parameterization: points are equally spaced.
        return 1.;
    }

public:
    // Harsh: need to reimplement parameterize method since CGAL doesnt offer what we need!
    template<typename VertexUVMap,
             typename VertexIndexMap,
             typename VertexParameterizedMap>
    SMP::Error_code parameterize(const TriangleMesh& mesh,
                                 halfedge_descriptor bhd,
                                 VertexUVMap uvmap,
                                 VertexIndexMap /*vimap */,
                                 VertexParameterizedMap vpmap)
    {
        // Nothing to do if no border
        if (bhd == halfedge_descriptor())
            return SMP::ERROR_BORDER_TOO_SHORT;

        // check the number of border edges
        std::size_t size_of_border = halfedges_around_face(bhd, mesh).size();
        if(size_of_border < 4)
            return SMP::ERROR_BORDER_TOO_SHORT;


        // for each border vertex, fix the parameterization
        BOOST_FOREACH(halfedge_descriptor hd, halfedges_around_face(bhd, mesh)) {

            vertex_descriptor vs = source(hd, mesh);
            const typename TriangleMesh::Point &p = mesh.point(vs);

            Kernel::Point_2 uv(p[0], p[1]);
            put(uvmap, vs, uv);
            put(vpmap, vs, true);
        }

        return SMP::OK;
    }
};

#endif
// parameterize the mesh
std::vector<TypeFunction> TriMesh::parameterize(bool verbose) {

#ifndef CGAL_AVAILABLE
    std::cerr << " ERROR: TriMesh::parameterize - CGAL not available! cannot sort vertices!\n";
    return std::vector<TypeFunction>();
#else

    // border parameterization
    typedef Square_border_projection_parameterizer_3<SurfaceMesh> Border_parameterizer;
    //typedef SMP::Square_border_uniform_parameterizer_3 <SurfaceMesh> Border_parameterizer;
    //typedef SMP::Square_border_arc_length_parameterizer_3<SurfaceMesh> Border_parameterizer;

    // surface paramterization
    typedef SMP::Discrete_authalic_parameterizer_3<SurfaceMesh, Border_parameterizer> Surface_parameterizer;
    //typedef SMP::Discrete_conformal_map_parameterizer_3<SurfaceMesh, Border_parameterizer> Surface_parameterizer;
    //typedef SMP::Mean_value_coordinates_parameterizer_3<SurfaceMesh, Border_parameterizer> Surface_parameterizer;

    // free boundary
    //typedef SMP::Two_vertices_parameterizer_3<SurfaceMesh> Border_parameterizer;
    //typedef SMP::ARAP_parameterizer_3<SurfaceMesh, Border_parameterizer> Surface_parameterizer;

    if (verbose) {
        std::cout << "   > TriMesh::parameterize(<" << mVertices.size() << ">)...";
        fflush(stdout);
    }

    // create a cgal mesh
    SurfaceMesh surface_mesh = to_cgal_mesh(mVertices, mFaces);

    // a halfedge on the border
    halfedge_descriptor bhd = CGAL::Polygon_mesh_processing::longest_border(surface_mesh).first;

    // The 2D points of the uv parametrisation will be written into this map
    UV_pmap uv_map = surface_mesh.add_property_map<vertex_descriptor, Kernel::Point_2>("h:uv").first;
    SMP::Error_code err = SMP::parameterize(surface_mesh, Surface_parameterizer(), bhd, uv_map);

    if(err != SMP::OK) {
        std::cerr << "Error: " << SMP::get_error_message(err) << std::endl;
        return std::vector<TypeFunction>();
    }

    const size_t nverts = mVertices.size();
    std::vector<TypeFunction> retval(2*nverts);
    size_t i = 0;
    for(auto pviter = uv_map.begin(); pviter != uv_map.end(); ++pviter, i++) {
        retval[2*i  ] = (*pviter)[0];
        retval[2*i+1] = (*pviter)[1];
    }

    if (verbose) {
        std::cout << " Done!\n";
    }
#endif
    return retval;
}


#ifdef CPP_REMESHING
#include <CGAL/Polygon_mesh_processing/remesh.h>

//! -----------------------------------------------------------------------------
//! remeshing
//! -----------------------------------------------------------------------------
struct halfedge2edge {

  halfedge2edge(const SurfaceMesh& m, std::vector<edge_descriptor>& edges)
    : m_mesh(m), m_edges(edges)
  {}
  void operator()(const halfedge_descriptor& h) const {
    m_edges.push_back(edge(h, m_mesh));
  }
  const SurfaceMesh& m_mesh;
  std::vector<edge_descriptor>& m_edges;
};

void TriMesh::remesh(bool verbose) {

    //TODO:!
    const double target_edge_length = 11;
    const unsigned int nb_iter = 3;

    SurfaceMesh mesh = to_cgal_mesh(mVertices, mFaces);

    if (verbose){
        std::cout << "Split border...";
        fflush(stdout);
    }

    std::vector<edge_descriptor> border;
    PMP::border_halfedges(CGAL::faces(mesh), mesh, boost::make_function_output_iterator(halfedge2edge(mesh, border)));
    PMP::split_long_edges(border, target_edge_length, mesh);

    if (verbose){
        std::cout << "done." << std::endl;
        std::cout << "Start remeshing (" << mesh.number_of_faces() << " faces, " << mesh.number_of_vertices() << " vertices)..." << std::endl;
    }
    PMP::isotropic_remeshing(CGAL::faces(mesh), target_edge_length, mesh,
                             PMP::parameters::number_of_iterations(nb_iter).protect_constraints(true) //i.e. protect border, here
                            );
    if (verbose){
        std::cout << "Remeshing done! (" << mesh.number_of_faces() << " faces, " << mesh.number_of_vertices() << " vertices)" << std::endl;
    }

    from_cgal_mesh(mesh, mVertices, mFaces);
}
#endif

//! -----------------------------------------------------------------------------
//! -----------------------------------------------------------------------------