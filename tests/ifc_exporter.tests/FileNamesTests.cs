using ifc_exporter;
using Xunit;

namespace ifc_exporter.tests
{
    /* Проверки подготовки имён выходных файлов IFC/NWC */
    public sealed class FileNamesTests
    {
        [Theory]
        [InlineData("CON")]
        [InlineData("con")]
        [InlineData("LPT9")]
        [InlineData("NUL.ifc")]
        public void IsReservedName_DetectsWindowsReservedNames(string name)
        {
            Assert.True(file_names.is_reserved_name(name));
        }

        [Theory]
        [InlineData("CONSOLE")]
        [InlineData("model")]
        [InlineData("COM10")]
        public void IsReservedName_AcceptsOrdinaryNames(string name)
        {
            Assert.False(file_names.is_reserved_name(name));
        }

        [Fact]
        public void Sanitize_ReplacesInvalidCharacters()
        {
            Assert.Equal("a_b_c_d_e", file_names.sanitize("a/b:c*d?e"));
        }

        [Fact]
        public void Sanitize_KeepsCyrillic()
        {
            Assert.Equal("Корпус 1_s_qNavisworks_v3D", file_names.sanitize("Корпус 1_s_qNavisworks_v3D"));
        }

        [Fact]
        public void Sanitize_TrimsSpacesAndDots()
        {
            Assert.Equal("model", file_names.sanitize("  model.. "));
        }

        [Fact]
        public void Sanitize_PrefixesReservedName()
        {
            Assert.Equal("_CON", file_names.sanitize("CON"));
        }

        [Fact]
        public void Sanitize_ReturnsEmptyForNullOrEmpty()
        {
            Assert.Equal(string.Empty, file_names.sanitize(null));
            Assert.Equal(string.Empty, file_names.sanitize(string.Empty));
        }

        [Fact]
        public void Sanitize_ReturnsPlaceholderWhenNothingLeft()
        {
            Assert.Equal("unnamed_file", file_names.sanitize(" ... "));
        }

        [Fact]
        public void Sanitize_LimitsLengthTo255()
        {
            Assert.Equal(255, file_names.sanitize(new string('a', 300)).Length);
        }
    }
}
