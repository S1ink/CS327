#include "spawning.hpp"

#include <iostream>

#include "util/sequential_parser.hpp"


bool MonDescription::parse(std::istream& f, std::vector<MonDescription>& descs)
{
    if(!MonDescription::verifyHeader(f)) return false;

    SequentialParser<MonDescription> parser;

    {
        parser.addStartToken("BEGIN MONSTER");
        parser.addEndToken("END");
        parser.addStringToken("NAME", MonDescription::Name);
        parser.addParagraphToken("DESC", MonDescription::Desc);
        parser.addAttributeToken<uint8_t>(
            "COLOR",
            MonDescription::Colors,
            {
                { "RED", DisplayColor::RED },
                { "GREEN", DisplayColor::GREEN },
                { "BLUE", DisplayColor::BLUE },
                { "CYAN", DisplayColor::CYAN },
                { "YELLOW", DisplayColor::YELLOW },
                { "MAGENTA", DisplayColor::MAGENTA },
                { "WHITE", DisplayColor::WHITE },
                { "BLACK", DisplayColor::BLACK }
            },
            true );
        parser.addRollableToken("SPEED", MonDescription::Speed);
        parser.addAttributeToken<uint16_t>(
            "ABIL",
            MonDescription::Abilities,
            {
                { "SMART", MonDescription::ABILITY_SMART },
                { "TELE", MonDescription::ABILITY_TELE },
                { "TUNNEL", MonDescription::ABILITY_TUNNEL },
                { "ERRATIC", MonDescription::ABILITY_ERRATIC },
                { "PASS", MonDescription::ABILITY_PASS },
                { "PICKUP", MonDescription::ABILITY_PICKUP },
                { "DESTROY", MonDescription::ABILITY_DESTROY },
                { "UNIQ", MonDescription::ABILITY_UNIQ },
                { "BOSS", MonDescription::ABILITY_BOSS },
            } );
        parser.addRollableToken("HP", MonDescription::Health);
        parser.addRollableToken("DAM", MonDescription::Attack);
        parser.addPrimitiveToken<char>("SYMB", MonDescription::Symbol);
        parser.addPrimitiveToken<uint8_t, int>("RRTY", MonDescription::Rarity);
    }

    parser.parse(f, descs);

    return true;
}

bool MonDescription::verifyHeader(std::istream& f)
{
    std::string line;
    getlineAndTrim(f, line);
    return line == "RLG327 MONSTER DESCRIPTION 1";
}

void MonDescription::serialize(std::ostream& out) const
{
    static const char* ABILITIES[] =
    {
        "SMART",
        "TELE",
        "TUNNEL",
        "ERRATIC",
        "PASS",
        "PICKUP",
        "DESTROY",
        "UNIQ",
        "BOSS"
    };
    static const char* COLORS[] =
    {
        "RED",
        "GREEN",
        "BLUE",
        "CYAN",
        "YELLOW",
        "MAGENTA",
        "WHITE",
        "BLACK"
    };

    out << "MonDescription@0x" << std::hex << reinterpret_cast<uintptr_t>(this) << std::dec
        << "\nName : " << this->name
        << "\nDesc : \"" << this->desc
        << "\"\nSymbol : " << this->symbol;

    out << "\nColors :";
    for(size_t i = 0; i < sizeof(COLORS) / sizeof(*COLORS); i++)
    {
        if(this->colors >> i & 0b1)
        {
            out << ' ' << COLORS[i];
        }
    }

    out << "\nSpeed : "; this->speed.serialize(out);
    out << "\nAbilities :";
    for(size_t i = 0; i < sizeof(ABILITIES) / sizeof(*ABILITIES); i++)
    {
        if(this->abilities >> i & 0b1)
        {
            std::cout << ' ' << ABILITIES[i];
        }
    }

    out << "\nHealth : "; this->health.serialize(out);
    out << "\nDamage : "; this->attack.serialize(out);
    out << "\nRarity : " << static_cast<int>(this->rarity) << '\n';

}




bool ItemDescription::parse(std::istream& f, std::vector<ItemDescription>& descs)
{
    if(!ItemDescription::verifyHeader(f)) return false;

    SequentialParser<ItemDescription> parser;

    {
        parser.addStartToken("BEGIN OBJECT");
        parser.addEndToken("END");
        parser.addStringToken("NAME", ItemDescription::Name);
        parser.addParagraphToken("DESC", ItemDescription::Desc);
        parser.addAttributeToken<uint32_t>(
            "TYPE",
            ItemDescription::Types,
            {
                { "WEAPON", ItemDescription::TYPE_WEAPON },
                { "OFFHAND", ItemDescription::TYPE_OFFHAND },
                { "RANGED", ItemDescription::TYPE_RANGED },
                { "ARMOR", ItemDescription::TYPE_ARMOR },
                { "HELMET", ItemDescription::TYPE_HELMET },
                { "CLOAK", ItemDescription::TYPE_CLOAK },
                { "GLOVES", ItemDescription::TYPE_GLOVES },
                { "BOOTS", ItemDescription::TYPE_BOOTS },
                { "RING", ItemDescription::TYPE_RING },
                { "AMULET", ItemDescription::TYPE_AMULET },
                { "LIGHT", ItemDescription::TYPE_LIGHT },
                { "SCROLL", ItemDescription::TYPE_SCROLL },
                { "BOOK", ItemDescription::TYPE_BOOK },
                { "FLASK", ItemDescription::TYPE_FLASK },
                { "GOLD", ItemDescription::TYPE_GOLD },
                { "AMMUNITION", ItemDescription::TYPE_AMMUNITION },
                { "FOOD", ItemDescription::TYPE_FOOD },
                { "WAND", ItemDescription::TYPE_WAND },
                { "CONTAINER", ItemDescription::TYPE_CONTAINER },
            } );
        parser.addAttributeToken<uint8_t>(
            "COLOR",
            ItemDescription::Colors,
            {
                { "RED", DisplayColor::RED },
                { "GREEN", DisplayColor::GREEN },
                { "BLUE", DisplayColor::BLUE },
                { "CYAN", DisplayColor::CYAN },
                { "YELLOW", DisplayColor::YELLOW },
                { "MAGENTA", DisplayColor::MAGENTA },
                { "WHITE", DisplayColor::WHITE },
                { "BLACK", DisplayColor::BLACK }
            },
            true );
        parser.addRollableToken("HIT", ItemDescription::Hit);
        parser.addRollableToken("DAM", ItemDescription::Damage);
        parser.addRollableToken("DODGE", ItemDescription::Dodge);
        parser.addRollableToken("DEF", ItemDescription::Defense);
        parser.addRollableToken("WEIGHT", ItemDescription::Weight);
        parser.addRollableToken("SPEED", ItemDescription::Speed);
        parser.addRollableToken("ATTR", ItemDescription::Special);
        parser.addRollableToken("VAL", ItemDescription::Value);
        parser.addAttributeToken<bool>("ART", ItemDescription::Artifact, { { "TRUE", true }, { "FALSE", false } });
        parser.addPrimitiveToken<uint8_t, int>("RRTY", ItemDescription::Rarity);
    }

    parser.parse(f, descs);

    return true;
}

bool ItemDescription::verifyHeader(std::istream& f)
{
    std::string line;
    getlineAndTrim(f, line);
    return line == "RLG327 OBJECT DESCRIPTION 1";
}

void ItemDescription::serialize(std::ostream& out) const
{
    static const char* TYPES[] =
    {
        "WEAPON",
        "OFFHAND",
        "RANGED",
        "ARMOR",
        "HELMET",
        "CLOAK",
        "GLOVES",
        "BOOTS",
        "RING",
        "AMULET",
        "LIGHT",
        "SCROLL",
        "BOOK",
        "FLASK",
        "GOLD",
        "AMMUNITION",
        "FOOD",
        "WAND",
        "CONTAINER"
    };
    static const char* COLORS[] =
    {
        "RED",
        "GREEN",
        "BLUE",
        "CYAN",
        "YELLOW",
        "MAGENTA",
        "WHITE",
        "BLACK"
    };

    out << "ItemDescription@0x" << std::hex << reinterpret_cast<uintptr_t>(this) << std::dec
        << "\nName : " << this->name
        << "\nDesc : \"" << this->desc;

    out << "\nTypes :";
    for(size_t i = 0; i < sizeof(TYPES) / sizeof(*TYPES); i++)
    {
        if(this->types >> i & 0b1)
        {
            out << ' ' << TYPES[i];
        }
    }

    out << "\nColors :";
    for(size_t i = 0; i < sizeof(COLORS) / sizeof(*COLORS); i++)
    {
        if(this->colors >> i & 0b1)
        {
            out << ' ' << COLORS[i];
        }
    }

    out << "\nHit : "; this->hit.serialize(out);
    out << "\nDamage : "; this->damage.serialize(out);
    out << "\nDodge : "; this->dodge.serialize(out);
    out << "\nDefense : "; this->defense.serialize(out);
    out << "\nWeight : "; this->weight.serialize(out);
    out << "\nSpeed : "; this->speed.serialize(out);
    out << "\nSpecial : "; this->special.serialize(out);
    out << "\nValue : "; this->value.serialize(out);

    out << "\nArtifact : " << (this->artifact ? "true" : "false")
        << "\nRarity : " << static_cast<int>(this->rarity) << '\n';
}










Entity::Entity(Entity::PCGenT x) :
    config
    {
        .name{ "Its you lol" },
        .desc{ "An unlikely hero." },
        .attack_damage{ { .base{ 8 }, .sides{ 6 }, .rolls{ 2 } }, std::random_device{}() },
        .speed{ 100 },
        .ability_bits{ 0 },
        .color{ DisplayColor::WHITE },
        .symbol{ '@' },
        .unique_entry{ nullptr }
    },
    state
    {
        .health{ 100 }
    }
{
    this->config.is_pc = 1;
}

Entity::Entity(const MonDescription& md, std::mt19937& gen) :
    config
    {
        .name{ md.name },
        .desc{ md.desc },
        .attack_damage{ md.attack, gen() },
        .speed{ md.speed.roll(gen()) },
        .ability_bits{ md.abilities },
        .color{ md.colors },
        .symbol{ md.symbol },
        .unique_entry{ (md.abilities & MonDescription::ABILITY_UNIQ) ? &md : nullptr }
    },
    state
    {
        .health{ md.health.roll(gen()) }
    }
{}

Entity::Entity(Entity&& e) :
    config
    {
        .name{ std::move(e.config.name) },
        .desc{ std::move(e.config.desc) },
        .attack_damage{ std::move(e.config.attack_damage) },
        .speed{ e.config.speed },
        .ability_bits{ e.config.ability_bits },
        .color{ e.config.color },
        .symbol{ e.config.symbol },
        .unique_entry{ e.config.unique_entry }
    },
    state
    {
        .pos{ e.state.pos },
        .target_pos{ e.state.target_pos },
        .health{ e.state.health }
    }
{
    e.config.unique_entry = nullptr;
}
Entity& Entity::operator=(Entity&& e)
{
    this->config.name = std::move(e.config.name);
    this->config.desc = std::move(e.config.desc);
    this->config.attack_damage = std::move(e.config.attack_damage);
    this->config.speed = e.config.speed;
    this->config.ability_bits = e.config.ability_bits;
    this->config.color = e.config.color;
    this->config.symbol = e.config.symbol;
    this->config.unique_entry = e.config.unique_entry;

    this->state.pos = e.state.pos;
    this->state.target_pos = e.state.target_pos;
    this->state.health = e.state.health;

    e.config.unique_entry = nullptr;
}
