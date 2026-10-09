#include <catch2/catch_test_macros.hpp>

#include "MediaTag.h"

TEST_CASE("MediaTag supports typed query and case-insensitive keys", "[core][mediatag]")
{
    CMediaTag tags;
    tags.AddTagString("Title", "Song A");
    tags.AddTagInteger("Track", 7);
    tags.AddTagDecimal("Duration", 12.5);

    REQUIRE(tags.QueryTagString("title").has_value());
    CHECK(tags.QueryTagString("title").value() == "Song A");
    REQUIRE(tags.QueryTagInteger("TRACK").has_value());
    CHECK(tags.QueryTagInteger("TRACK").value() == 7);
    REQUIRE(tags.QueryTagDecimal("duration").has_value());
    CHECK(tags.QueryTagDecimal("duration").value() == 12.5);
}

TEST_CASE("MediaTag returns empty optional for type mismatch or missing", "[core][mediatag]")
{
    CMediaTag tags;
    tags.AddTagString("Artist", "Someone");

    CHECK_FALSE(tags.QueryTagInteger("Artist").has_value());
    CHECK_FALSE(tags.QueryTagDecimal("Artist").has_value());
    CHECK_FALSE(tags.QueryTagString("NotExist").has_value());
}

TEST_CASE("MediaTag clear resets all tags", "[core][mediatag]")
{
    CMediaTag tags;
    CHECK(tags.IsEmpty());

    tags.AddTagString("album", "A");
    CHECK_FALSE(tags.IsEmpty());

    tags.Clear();
    CHECK(tags.IsEmpty());
    CHECK_FALSE(tags.QueryTagString("album").has_value());
}
