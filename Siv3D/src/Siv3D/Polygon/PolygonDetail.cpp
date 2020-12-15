﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2020 Ryo Suzuki
//	Copyright (c) 2016-2020 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include "PolygonDetail.hpp"
# include "Triangulation.hpp"
# include <Siv3D/Renderer2D/IRenderer2D.hpp>
# include <Siv3D/Common/Siv3DEngine.hpp>

SIV3D_DISABLE_MSVC_WARNINGS_PUSH(4100)
SIV3D_DISABLE_MSVC_WARNINGS_PUSH(4244)
SIV3D_DISABLE_MSVC_WARNINGS_PUSH(4819)
# include <boost/geometry/strategies/strategies.hpp>
# include <boost/geometry/algorithms/centroid.hpp>
# include <boost/geometry/algorithms/convex_hull.hpp>
# include <boost/geometry/algorithms/simplify.hpp>
# include <boost/geometry/algorithms/buffer.hpp>
SIV3D_DISABLE_MSVC_WARNINGS_POP()
SIV3D_DISABLE_MSVC_WARNINGS_POP()
SIV3D_DISABLE_MSVC_WARNINGS_POP()

namespace s3d
{
	namespace detail
	{
		template <class Type>
		static RectF CalculateBoundingRect(const Vector2D<Type>* const pVertex, const size_t vertexSize)
		{
			assert(pVertex != nullptr);
			assert(vertexSize != 0);

			const Vector2D<Type>* it = pVertex;
			const Vector2D<Type>* itEnd = (it + vertexSize);

			double left		= it->x;
			double top		= it->y;
			double right	= left;
			double bottom	= top;
			++it;

			while (it != itEnd)
			{
				if (it->x < left)
				{
					left = it->x;
				}
				else if (right < it->x)
				{
					right = it->x;
				}

				if (it->y < top)
				{
					top = it->y;
				}
				else if (bottom < it->y)
				{
					bottom = it->y;
				}

				++it;
			}

			return{ left, top, (right - left), (bottom - top) };
		}

		[[nodiscard]]
		static double TriangleArea(const Float2& p0, const Float2& p1, const Float2& p2) noexcept
		{
			return std::abs((p0.x - p2.x) * (p1.y - p0.y) - (p0.x - p1.x) * (p2.y - p0.y)) * 0.5;
		}
	}

	Polygon::PolygonDetail::PolygonDetail()
	{

	}

	Polygon::PolygonDetail::PolygonDetail(const Vec2* const pOuterVertex, const size_t vertexSize, Array<Array<Vec2>> holes, const SkipValidation skipValidation)
	{
		if (vertexSize < 3)
		{
			return;
		}

		if (not skipValidation)
		{
			if (Validate(pOuterVertex, vertexSize, holes)
				!= PolygonFailureType::OK)
			{
				return;
			}
		}

		holes.remove_if([](const Array<Vec2>& hole) { return (hole.size() < 3); });

		// [1 of 5]
		{
			m_polygon.outer().assign(pOuterVertex, pOuterVertex + vertexSize);

			for (const auto& hole : holes)
			{
				m_polygon.inners().emplace_back(hole.begin(), hole.end());
			}
		}

		// [2 of 5]
		m_holes = std::move(holes);

		// [3 of 5], [4 of 5]
		{
			Array<Vertex2D::IndexType> indices;
			detail::Triangulate(m_polygon.outer(), m_holes, m_vertices, indices);
			assert(indices.size() % 3 == 0);
			m_indices.resize(indices.size() / 3);
			assert(m_indices.size_bytes() == indices.size_bytes());
			std::memcpy(m_indices.data(), indices.data(), indices.size_bytes());
		}

		// [5 of 5]
		m_boundingRect = detail::CalculateBoundingRect(pOuterVertex, vertexSize);
	}

	Polygon::PolygonDetail::PolygonDetail(const Vec2* pOuterVertex, const size_t vertexSize, Array<TriangleIndex> indices, const RectF& boundingRect, const SkipValidation skipValidation)
	{
		if (vertexSize < 3)
		{
			return;
		}

		if (not skipValidation)
		{
			if (Validate(pOuterVertex, vertexSize)
				!= PolygonFailureType::OK)
			{
				return;
			}
		}

		// [1 of 5]
		m_polygon.outer().assign(pOuterVertex, pOuterVertex + vertexSize);

		// [2 of 5]
		// do nothing

		// [3 of 5]
		m_vertices.assign(pOuterVertex, pOuterVertex + vertexSize);

		// [4 of 5]
		m_indices = std::move(indices);

		// [5 of 5]
		m_boundingRect = boundingRect;
	}

	Polygon::PolygonDetail::PolygonDetail(const Array<Vec2>& outer, Array<Array<Vec2>> holes, Array<Float2> vertices, Array<TriangleIndex> indices, const RectF& boundingRect, const SkipValidation skipValidation)
	{
		if (outer.size() < 3)
		{
			return;
		}

		if (not skipValidation)
		{
			if (Validate(outer, holes)
				!= PolygonFailureType::OK)
			{
				return;
			}
		}

		holes.remove_if([](const Array<Vec2>& hole) { return (hole.size() < 3); });

		// [1 of 5]
		{
			m_polygon.outer().assign(outer.begin(), outer.end());

			for (const auto& hole : holes)
			{
				m_polygon.inners().emplace_back(hole.begin(), hole.end());
			}
		}

		// [2 of 5]
		m_holes = std::move(holes);

		// [3 of 5]
		m_vertices = std::move(vertices);

		// [4 of 5]
		m_indices = std::move(indices);

		// [5 of 5]
		m_boundingRect = boundingRect;
	}

	Polygon::PolygonDetail::PolygonDetail(const Float2* pOuterVertex, const size_t vertexSize, Array<TriangleIndex> indices)
	{
		if (vertexSize < 3)
		{
			return;
		}

		// [1 of 5]
		m_polygon.outer().assign(pOuterVertex, pOuterVertex + vertexSize);

		// [2 of 5]
		// do nothing

		// [3 of 5]
		m_vertices.assign(pOuterVertex, pOuterVertex + vertexSize);

		// [4 of 5]
		m_indices = std::move(indices);

		// [5 of 5]
		m_boundingRect = detail::CalculateBoundingRect(pOuterVertex, vertexSize);
	}

	const Array<Vec2>& Polygon::PolygonDetail::outer() const noexcept
	{
		return m_polygon.outer();
	}

	const Array<Array<Vec2>>& Polygon::PolygonDetail::inners() const noexcept
	{
		return m_holes;
	}

	const Array<Float2>& Polygon::PolygonDetail::vertices() const noexcept
	{
		return m_vertices;
	}

	const Array<TriangleIndex>& Polygon::PolygonDetail::indices() const noexcept
	{
		return m_indices;
	}

	const RectF& Polygon::PolygonDetail::boundingRect() const noexcept
	{
		return m_boundingRect;
	}

	void Polygon::PolygonDetail::moveBy(const Vec2 v) noexcept
	{
		if (outer().isEmpty())
		{
			return;
		}

		{
			for (auto& point : m_polygon.outer())
			{
				point.moveBy(v);
			}

			for (auto& hole : m_polygon.inners())
			{
				for (auto& point : hole)
				{
					point.moveBy(v);
				}
			}
		}

		for (auto& hole : m_holes)
		{
			for (auto& point : hole)
			{
				point.moveBy(v);
			}
		}

		{
			const Float2 vf{ v };

			for (auto& point : m_vertices)
			{
				point.moveBy(vf);
			}
		}

		m_boundingRect.moveBy(v);
	}




	double Polygon::PolygonDetail::area() const noexcept
	{
		const size_t _num_triangles = m_indices.size();

		double result = 0.0;

		for (size_t i = 0; i < _num_triangles; ++i)
		{
			const auto& index = m_indices[i];

			result += detail::TriangleArea(m_vertices[index.i0], m_vertices[index.i1], m_vertices[index.i2]);
		}

		return result;
	}

	double Polygon::PolygonDetail::perimeter() const noexcept
	{
		double result = 0.0;

		{
			const auto& outer = m_polygon.outer();
			const size_t num_outer = outer.size();

			for (size_t i = 0; i < num_outer; ++i)
			{
				result += outer[i].distanceFrom(outer[(i + 1) % num_outer]);
			}
		}

		{
			for (const auto& inner : m_polygon.inners())
			{
				const size_t num_inner = inner.size();

				for (size_t i = 0; i < num_inner; ++i)
				{
					result += inner[i].distanceFrom(inner[(i + 1) % num_inner]);
				}
			}
		}

		return result;
	}

	Vec2 Polygon::PolygonDetail::centroid() const
	{
		if (outer().isEmpty())
		{
			return Vec2{ 0, 0 };
		}

		Vec2 centroid;

		boost::geometry::centroid(m_polygon, centroid);

		return centroid;
	}

	Polygon Polygon::PolygonDetail::calculateConvexHull() const
	{
		CWOpenRing result;

		boost::geometry::convex_hull(m_polygon.outer(), result);

		return Polygon{ result };
	}

	Polygon Polygon::PolygonDetail::calculateBuffer(const double distance) const
	{
		using polygon_t = boost::geometry::model::polygon<Vec2, true, false>;
		const boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(distance);
		const boost::geometry::strategy::buffer::end_round end_strategy(0);
		const boost::geometry::strategy::buffer::point_circle circle_strategy(0);
		const boost::geometry::strategy::buffer::side_straight side_strategy;
		const boost::geometry::strategy::buffer::join_miter join_strategy;

		const auto& src = m_polygon;

		polygon_t in;
		{
			for (size_t i = 0; i < src.outer().size(); ++i)
			{
				in.outer().push_back(src.outer()[src.outer().size() - i - 1]);
			}

			if (src.outer().size() >= 2)
			{
				in.outer().push_back(src.outer()[src.outer().size() - 1]);

				in.outer().push_back(src.outer()[src.outer().size() - 2]);
			}
		}

		if (const size_t num_holes = src.inners().size())
		{
			in.inners().resize(num_holes);

			for (size_t i = 0; i < num_holes; ++i)
			{
				for (size_t k = 0; k < src.inners()[i].size(); ++k)
				{
					in.inners()[i].push_back(src.inners()[i][src.inners()[i].size() - k - 1]);
				}
			}
		}

		boost::geometry::model::multi_polygon<CwOpenPolygon> multiPolygon;
		boost::geometry::buffer(in, multiPolygon, distance_strategy, side_strategy, join_strategy, end_strategy, circle_strategy);

		if (multiPolygon.size() != 1)
		{
			return{};
		}

		auto& outer = multiPolygon[0].outer();

		if (outer.size() > 2 && (outer.front().x == outer.back().x) && (outer.front().y == outer.back().y))
		{
			outer.pop_back();
		}

		const auto& inners = multiPolygon[0].inners();

		Array<Array<Vec2>> holes(inners.size());

		for (size_t i = 0; i < holes.size(); ++i)
		{
			const auto& resultHole = inners[i];

			holes[i].assign(resultHole.rbegin(), resultHole.rend());
		}

		return Polygon{ outer, holes };
	}

	Polygon Polygon::PolygonDetail::calculateRoundBuffer(const double distance) const
	{
		using polygon_t = boost::geometry::model::polygon<Vec2, true, false>;
		const boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(distance);
		const boost::geometry::strategy::buffer::end_round end_strategy(0);
		const boost::geometry::strategy::buffer::point_circle circle_strategy(0);
		const boost::geometry::strategy::buffer::side_straight side_strategy;
		const boost::geometry::strategy::buffer::join_round_by_divide join_strategy(4);

		const auto& src = m_polygon;

		polygon_t in;
		{
			for (size_t i = 0; i < src.outer().size(); ++i)
			{
				in.outer().push_back(src.outer()[src.outer().size() - i - 1]);
			}

			if (src.outer().size() >= 2)
			{
				in.outer().push_back(src.outer()[src.outer().size() - 1]);

				in.outer().push_back(src.outer()[src.outer().size() - 2]);
			}
		}

		if (const size_t num_holes = src.inners().size())
		{
			in.inners().resize(num_holes);

			for (size_t i = 0; i < num_holes; ++i)
			{
				for (size_t k = 0; k < src.inners()[i].size(); ++k)
				{
					in.inners()[i].push_back(src.inners()[i][src.inners()[i].size() - k - 1]);
				}
			}
		}

		boost::geometry::model::multi_polygon<CwOpenPolygon> multiPolygon;
		boost::geometry::buffer(in, multiPolygon, distance_strategy, side_strategy, join_strategy, end_strategy, circle_strategy);

		if (multiPolygon.size() != 1)
		{
			return{};
		}

		auto& outer = multiPolygon[0].outer();

		if (outer.size() > 2 && (outer.front().x == outer.back().x) && (outer.front().y == outer.back().y))
		{
			outer.pop_back();
		}

		const auto& inners = multiPolygon[0].inners();

		Array<Array<Vec2>> holes(inners.size());

		for (size_t i = 0; i < holes.size(); ++i)
		{
			const auto& resultHole = inners[i];

			holes[i].assign(resultHole.rbegin(), resultHole.rend());
		}

		return Polygon{ outer, holes };
	}

	Polygon Polygon::PolygonDetail::simplified(const double maxDistance) const
	{
		if (!m_polygon.outer())
		{
			return{};
		}

		GLineString result;
		{
			GLineString v(m_polygon.outer().begin(), m_polygon.outer().end());

			v.push_back(v.front());

			boost::geometry::simplify(v, result, maxDistance);

			if (result.size() > 3)
			{
				result.pop_back();
			}
		}

		Array<Array<Vec2>> holeResults;

		for (auto& hole : m_polygon.inners())
		{
			GLineString v(hole.begin(), hole.end()), result2;

			v.push_back(v.front());

			boost::geometry::simplify(v, result2, maxDistance);

			if (result2.size() > 3)
			{
				result2.pop_back();
			}

			holeResults.push_back(std::move(result2));
		}

		return Polygon{ result, holeResults };
	}



	void Polygon::PolygonDetail::draw(const ColorF& color) const
	{
		SIV3D_ENGINE(Renderer2D)->addPolygon(m_vertices, m_indices, none, color.toFloat4());
	}

	void Polygon::PolygonDetail::drawFrame(const double thickness, const ColorF& color) const
	{
		if (not m_polygon.outer())
		{
			return;
		}

		SIV3D_ENGINE(Renderer2D)->addLineString(
			m_polygon.outer().data(),
			m_polygon.outer().size(),
			none,
			static_cast<float>(thickness),
			false,
			color.toFloat4(),
			IsClosed::Yes
		);

		for (const auto& hole : m_polygon.inners())
		{
			SIV3D_ENGINE(Renderer2D)->addLineString(
				hole.data(),
				hole.size(),
				none,
				static_cast<float>(thickness),
				false,
				color.toFloat4(),
				IsClosed::Yes
			);
		}
	}
}
