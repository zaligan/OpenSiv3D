﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2021 Ryo Suzuki
//	Copyright (c) 2016-2021 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include <Siv3D/Functor.hpp>
# include "NavMeshDetail.hpp"

namespace s3d
{
	namespace detail
	{
		[[nodiscard]]
		static NavMeshAABB CalculateAABB(const Array<Float2>& vertices) noexcept
		{
			NavMeshAABB aabb;

			aabb.bmin[1] = 0.0f;
			aabb.bmax[1] = 0.0f;

			for (const auto& vertex : vertices)
			{
				aabb.bmin[0] = Min(aabb.bmin[0], vertex.x);
				aabb.bmin[2] = Min(aabb.bmin[2], vertex.y);

				aabb.bmax[0] = Max(aabb.bmax[0], vertex.x);
				aabb.bmax[2] = Max(aabb.bmax[2], vertex.y);
			}

			return aabb;
		}

		[[nodiscard]]
		static NavMeshAABB CalculateAABB(const Array<Float3>& vertices) noexcept
		{
			NavMeshAABB aabb;

			for (const auto& vertex : vertices)
			{
				aabb.bmin[0] = Min(aabb.bmin[0], vertex.x);
				aabb.bmin[1] = Min(aabb.bmin[1], vertex.y);
				aabb.bmin[2] = Min(aabb.bmin[2], vertex.z);

				aabb.bmax[0] = Max(aabb.bmax[0], vertex.x);
				aabb.bmax[1] = Max(aabb.bmax[1], vertex.y);
				aabb.bmax[2] = Max(aabb.bmax[2], vertex.z);
			}

			return aabb;
		}

		[[nodiscard]]
		rcConfig MakeConfig(const NavMeshConfig& config, const NavMeshAABB& aabb)
		{
			const float cellSize		= static_cast<float>(config.cellSize);
			const float cellHeight		= static_cast<float>(config.cellHeight);
			const float agentMaxSlope	= static_cast<float>(config.agentMaxSlope);
			const float agentHeight		= static_cast<float>(config.agentHeight);
			const float agentMaxClimb	= static_cast<float>(config.agentMaxClimb);
			const float agentRadius		= static_cast<float>(config.agentRadius);

			constexpr float edgeMaxLen				= 12.0f;
			constexpr float detailSampleDist		= 6.0f;
			constexpr float detailSampleMaxError	= 1.0f;
			constexpr float regionMinSize			= 8.0f;
			constexpr float regionMergeSize			= 20.0f;

			rcConfig cfg
			{
				.cs						= cellSize,
				.ch						= cellHeight,
				.walkableSlopeAngle		= agentMaxSlope,
				.walkableHeight			= static_cast<int32>(std::ceil(agentHeight / cellHeight)),
				.walkableClimb			= static_cast<int32>(std::floor(agentMaxClimb / cellHeight)),
				.walkableRadius			= static_cast<int32>(std::ceil(agentRadius / cellSize)),
				.maxEdgeLen				= static_cast<int32>(edgeMaxLen / cellSize),
				.maxSimplificationError	= 1.3f,
				.minRegionArea			= static_cast<int32>(regionMinSize * regionMinSize),
				.mergeRegionArea		= static_cast<int32>(regionMergeSize * regionMergeSize),
				.maxVertsPerPoly		= 6,
				.detailSampleDist		= (detailSampleDist < 0.9f) ? 0 : cellSize * detailSampleDist,
				.detailSampleMaxError	= cellHeight * detailSampleMaxError,
			};

			rcVcopy(cfg.bmin, aabb.bmin);
			rcVcopy(cfg.bmax, aabb.bmax);
			cfg.bmin[0] -= cfg.borderSize * cellSize;
			cfg.bmin[2] -= cfg.borderSize * cellSize;
			cfg.bmax[0] += cfg.borderSize * cellSize;
			cfg.bmax[2] += cfg.borderSize * cellSize;

			rcCalcGridSize(cfg.bmin, cfg.bmax, cellSize, &cfg.width, &cfg.height);

			cfg.width	= static_cast<int32>((cfg.bmax[0] - cfg.bmin[0]) / cellSize + 1);
			cfg.height	= static_cast<int32>((cfg.bmax[2] - cfg.bmin[2]) / cellSize + 1);

			return cfg;
		}
	}

	NavMesh::NavMeshDetail::NavMeshDetail()
	{
		// do nothing
	}

	NavMesh::NavMeshDetail::~NavMeshDetail()
	{
		release();
	}

	bool NavMesh::NavMeshDetail::build(const Array<Float2>& vertices, const Array<TriangleIndex>& indices, const Array<uint8>& areaIDs, const NavMeshConfig& config)
	{
		release();

		if ((not vertices)
			|| (not indices)
			|| (not areaIDs))
		{
			return false;
		}

		if (indices.size() != areaIDs.size())
		{
			return false;
		}

		if (not areaIDs.all(LessThanEqual(RC_WALKABLE_AREA)))
		{
			return false;
		}

		try
		{
			const Array<Float3> vertex3 = vertices.map([](const Float2& v) { return Float3{ v.x, 0.0f, v.y }; });
			build(config, detail::CalculateAABB(vertices), vertex3, indices, areaIDs);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	bool NavMesh::NavMeshDetail::build(const Array<Float3>& vertices, const Array<TriangleIndex>& indices, const Array<uint8>& areaIDs, const NavMeshConfig& config)
	{
		release();

		if ((not vertices)
			|| (not indices)
			|| (not areaIDs))
		{
			return false;
		}

		if (indices.size() != areaIDs.size())
		{
			return false;
		}

		if (not areaIDs.all(LessThanEqual(RC_WALKABLE_AREA)))
		{
			return false;
		}

		try
		{
			build(config, detail::CalculateAABB(vertices), vertices, indices, areaIDs);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	Array<Vec2> NavMesh::NavMeshDetail::query(const Float2& _start, const Float2& _end, const Array<std::pair<int, double>>& areaCosts) const
	{
		if (not m_built)
		{
			return{};
		}

		dtQueryFilter filter;
		{
			for (const auto& areaCost : areaCosts)
			{
				if (areaCost.first <= RC_WALKABLE_AREA)
				{
					filter.setAreaCost(areaCost.first, static_cast<float>(areaCost.second));
				}
			}
		}

		const Float3 start{ _start.x, 0.0f, _start.y }, end{ _end.x, 0.0f, _end.y };
		constexpr Float3 extent{ 2.0f, 0.0f, 2.0f };

		dtPolyRef startpoly;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findNearestPoly(&start.x, &extent.x, &filter, &startpoly, 0)))
			{
				return{};
			}

			if (startpoly == 0)
			{
				return{};
			}
		}

		dtPolyRef endpoly;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findNearestPoly(&end.x, &extent.x, &filter, &endpoly, 0)))
			{
				return{};
			}

			if (endpoly == 0)
			{
				return{};
			}
		}

		constexpr int32 buffersize = 8192;
		Array<dtPolyRef> polys(buffersize);
		int32 npolys = 0;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findPath(startpoly, endpoly, &start.x, &end.x, &filter, polys.data(), &npolys, buffersize)))
			{
				return{};
			}

			if (npolys <= 0)
			{
				return{};
			}
		}

		float end2[3] = { end.x, end.y, end.z };

		if (polys[static_cast<size_t>(npolys) - 1] != endpoly)
		{
			bool posOverPoly;
			m_data.navmeshQuery.closestPointOnPoly(polys[static_cast<size_t>(npolys) - 1], &end.x, end2, &posOverPoly);
		}

		constexpr int32 maxvertices = 8192;

		Array<Float3> buffer(maxvertices);
		{
			int32 nvertices = 0;

			m_data.navmeshQuery.findStraightPath(&start.x, end2, polys.data(), npolys, &buffer[0].x, 0, 0, &nvertices, maxvertices);

			buffer.resize(nvertices);
		}

		return buffer.map([](const Float3& v) { return Vec2{ v.x, v.z }; });
	}

	Array<Vec3> NavMesh::NavMeshDetail::query(const Float3& start, const Float3& end, const Array<std::pair<int, double>>& areaCosts) const
	{
		if (not m_built)
		{
			return{};
		}

		dtQueryFilter filter;
		{
			for (const auto& areaCost : areaCosts)
			{
				if (areaCost.first <= RC_WALKABLE_AREA)
				{
					filter.setAreaCost(areaCost.first, static_cast<float>(areaCost.second));
				}
			}
		}

		constexpr Float3 extent{ 2.0f, 4.0f, 2.0f };

		dtPolyRef startpoly;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findNearestPoly(&start.x, &extent.x, &filter, &startpoly, 0)))
			{
				return{};
			}

			if (startpoly == 0)
			{
				return{};
			}
		}

		dtPolyRef endpoly;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findNearestPoly(&end.x, &extent.x, &filter, &endpoly, 0)))
			{
				return{};
			}

			if (endpoly == 0)
			{
				return{};
			}
		}

		constexpr int32 buffersize = 8192;
		Array<dtPolyRef> polys(buffersize);
		int32 npolys = 0;
		{
			if (dtStatusFailed(m_data.navmeshQuery.findPath(startpoly, endpoly, &start.x, &end.x, &filter, polys.data(), &npolys, buffersize)))
			{
				return{};
			}

			if (npolys <= 0)
			{
				return{};
			}
		}

		float end2[3] = { end.x, end.y, end.z };

		if (polys[static_cast<size_t>(npolys) - 1] != endpoly)
		{
			bool posOverPoly;
			m_data.navmeshQuery.closestPointOnPoly(polys[static_cast<size_t>(npolys) - 1], &end.x, end2, &posOverPoly);
		}

		constexpr int32 maxvertices = 8192;

		Array<Float3> buffer(maxvertices);
		{
			int32 nvertices = 0;

			m_data.navmeshQuery.findStraightPath(&start.x, end2, polys.data(), npolys, &buffer[0].x, 0, 0, &nvertices, maxvertices);

			buffer.resize(nvertices);
		}

		return buffer.map([](const Float3& v) { return Vec3{ v }; });
	}

	bool NavMesh::NavMeshDetail::build(const NavMeshConfig& config, const NavMeshAABB& aabb,
		const Array<Float3>& vertices, const Array<TriangleIndex>& indices, const Array<uint8>& areaIDs)
	{
		assert(not m_built);

		init();

		const rcConfig cfg = detail::MakeConfig(config, aabb);

		if (not rcCreateHeightfield(&m_data.ctx, *m_data.hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
		{
			return false;
		}

		const int32 flagMergeThreshold = 0;

		rcRasterizeTriangles(&m_data.ctx, &vertices[0].x, static_cast<int32>(vertices.size()),
			&(indices.front().i0), areaIDs.data(), static_cast<int32>(areaIDs.size()), *m_data.hf, flagMergeThreshold);

		rcFilterLowHangingWalkableObstacles(&m_data.ctx, cfg.walkableClimb, *m_data.hf);
		rcFilterLedgeSpans(&m_data.ctx, cfg.walkableHeight, cfg.walkableClimb, *m_data.hf);
		rcFilterWalkableLowHeightSpans(&m_data.ctx, cfg.walkableHeight, *m_data.hf);

		if (not rcBuildCompactHeightfield(&m_data.ctx, cfg.walkableHeight, cfg.walkableClimb, *m_data.hf, *m_data.chf))
		{
			return false;
		}

		if (not rcErodeWalkableArea(&m_data.ctx, cfg.walkableRadius, *m_data.chf))
		{
			return false;
		}

		if (not rcBuildDistanceField(&m_data.ctx, *m_data.chf))
		{
			return false;
		}

		if (not rcBuildRegions(&m_data.ctx, *m_data.chf, 0 /* border size */, cfg.minRegionArea, cfg.mergeRegionArea))
		{
			return false;
		}

		if (not rcBuildContours(&m_data.ctx, *m_data.chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *m_data.cset))
		{
			return false;
		}

		if (not rcBuildPolyMesh(&m_data.ctx, *m_data.cset, cfg.maxVertsPerPoly, *m_data.mesh))
		{
			return false;
		}

		if (not rcBuildPolyMeshDetail(&m_data.ctx, *m_data.mesh, *m_data.chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *m_data.dmesh))
		{
			return false;
		}

		const auto& mesh	= *m_data.mesh;
		const auto& dmesh	= *m_data.dmesh;

		for (int32 i = 0; i < mesh.npolys; ++i)
		{
			mesh.flags[i] = 1;
		}

		dtNavMeshCreateParams params;
		std::memset(&params, 0, sizeof(params));

		params.verts		= mesh.verts;
		params.vertCount	= mesh.nverts;
		params.polys		= mesh.polys;
		params.polyAreas	= mesh.areas;
		params.polyFlags	= mesh.flags;
		params.polyCount	= mesh.npolys;
		params.nvp			= mesh.nvp;

		params.detailMeshes		= dmesh.meshes;
		params.detailVerts		= dmesh.verts;
		params.detailVertsCount	= dmesh.nverts;
		params.detailTris		= dmesh.tris;
		params.detailTriCount	= dmesh.ntris;

		params.walkableHeight	= static_cast<float>(cfg.walkableHeight);
		params.walkableClimb	= static_cast<float>(cfg.walkableClimb);
		rcVcopy(params.bmin, mesh.bmin);
		rcVcopy(params.bmax, mesh.bmax);
		params.cs = cfg.cs;
		params.ch = cfg.ch;
		params.buildBvTree = true;

		if (not dtCreateNavMeshData(&params, &m_navData, &m_navDataSize))
		{
			return false;
		}

		m_data.navmesh->init(m_navData, m_navDataSize, DT_TILE_FREE_DATA);

		if (dtStatusFailed(m_data.navmeshQuery.init(m_data.navmesh.get(), 2048)))
		{
			return false;
		}

		m_built = true;

		return true;
	}

	void NavMesh::NavMeshDetail::init()
	{
		try
		{
			if (m_data.hf = rcAllocHeightfield(); 
				not m_data.hf)
			{
				throw std::bad_alloc();
			}

			if (m_data.chf = rcAllocCompactHeightfield();
				not m_data.chf)
			{
				throw std::bad_alloc();
			}

			if (m_data.cset = rcAllocContourSet(); 
				not m_data.cset)
			{
				throw std::bad_alloc();
			}

			if (m_data.mesh = rcAllocPolyMesh();
				not m_data.mesh)
			{
				throw std::bad_alloc();
			}

			if (m_data.dmesh = rcAllocPolyMeshDetail();
				not m_data.dmesh)
			{
				throw std::bad_alloc();
			}

			m_data.navmesh = std::shared_ptr<dtNavMesh>(dtAllocNavMesh(), dtFreeNavMesh);
		}
		catch (...)
		{
			release();

			throw;
		}
	}

	void NavMesh::NavMeshDetail::release()
	{
		if (not m_built)
		{
			return;
		}

		m_data.navmesh.reset();

		if (m_data.dmesh)
		{
			rcFreePolyMeshDetail(m_data.dmesh);
			m_data.dmesh = nullptr;
		}

		if (m_data.mesh)
		{
			rcFreePolyMesh(m_data.mesh);
			m_data.mesh = nullptr;
		}

		if (m_data.cset)
		{
			rcFreeContourSet(m_data.cset);
			m_data.cset = nullptr;
		}

		if (m_data.chf)
		{
			rcFreeCompactHeightfield(m_data.chf);
			m_data.chf = nullptr;
		}

		if (m_data.hf)
		{
			rcFreeHeightField(m_data.hf);
			m_data.hf = nullptr;
		}

		m_built = false;
	}
}
